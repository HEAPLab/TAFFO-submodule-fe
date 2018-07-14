#include <cassert>
#include <utility>
#include <map>

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"

#include "llvm/Transforms/ErrorPropagator/AffineForms.h"
#include "llvm/Transforms/ErrorPropagator/Metadata.h"
#include "RangeErrorMap.h"
#include "FunctionCopyMap.h"
#include "Propagators.h"

using namespace llvm;

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

cl::opt<unsigned> DefaultUnrollCount("dunroll",
				     cl::desc("Default loop unroll count"),
				     cl::value_desc("count"),
				     cl::init(1U));
cl::opt<bool> NoLoopUnroll("nounroll",
			   cl::desc("Never unroll loops"),
			   cl::init(false));
cl::opt<unsigned> CmpErrorThreshold("cmpthresh",
				    cl::desc("CMP errors are signaled"
					     "only if error is above perc %"),
				    cl::value_desc("perc"),
				    cl::init(0U));
cl::opt<unsigned> MaxRecursionCount("recur",
				    cl::desc("Default number of recursive calls"
					     "to the same function."),
				    cl::value_desc("count"),
				    cl::init(1U));

class ErrorPropagator : public ModulePass {
public:
  static char ID;
  ErrorPropagator() : ModulePass(ID) {}

  bool runOnModule(Module &) override;

  void getAnalysisUsage(AnalysisUsage &) const override;

protected:
  void retrieveGlobalVariablesRangeError(const Module &M, RangeErrorMap &RMap);

  void computeErrorsWithCopy(Function &F, RangeErrorMap &RMap,
			     FunctionCopyManager &FCMap,
			     SmallVectorImpl<Value *> *Args = nullptr,
			     bool GenMetadata = false);

  void computeErrors(Function &F, RangeErrorMap &RMap,
		     CmpErrorMap &CMpMap,
		     FunctionCopyManager &FCMap,
		     MemorySSA &MemSSA,
		     SmallVectorImpl<Value *> *ArgErrs);

  void computeErrors(Instruction &, RangeErrorMap &, CmpErrorMap &,
		     FunctionCopyManager &FCMap, MemorySSA &MemSSA);

  void dispatchInstruction(Instruction &, RangeErrorMap &, CmpErrorMap &,
			   FunctionCopyManager &FCMap, MemorySSA &MemSSA);

  void prepareErrorsForCall(RangeErrorMap &RMap,
			    CmpErrorMap &CmpMap,
			    FunctionCopyManager &FCMap,
			    Instruction &I);

  void attachErrorMetadata(Function &, const RangeErrorMap &,
			   const CmpErrorMap &, ValueToValueMapTy &);

  void checkCommandLine();
}; // end of class ErrorPropagator


bool ErrorPropagator::runOnModule(Module &M) {
  checkCommandLine();

  RangeErrorMap GlobalRMap;

  // Get Ranges and initial Errors for global variables.
  retrieveGlobalVariablesRangeError(M, GlobalRMap);

  // Copy list of original functions, so we don't mess up with copies.
  SmallVector<Function *, 4U> Functions;
  Functions.reserve(M.size());
  for (Function &F : M) {
    Functions.push_back(&F);
  }

  TargetLibraryInfo &TLI = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  FunctionCopyManager FCMap(MaxRecursionCount, DefaultUnrollCount,
			    NoLoopUnroll, TLI);

  // Iterate over all functions in this Module,
  // and propagate errors for pending input intervals for all of them.
  for (Function *F : Functions) {
    computeErrorsWithCopy(*F, GlobalRMap, FCMap, nullptr, true);
  }

  return false;
}

void ErrorPropagator::retrieveGlobalVariablesRangeError(const Module &M,
							RangeErrorMap &RMap) {
  for (const GlobalVariable &GV : M.globals()) {
    RMap.retrieveRangeError(GV);
  }
}

void ErrorPropagator::computeErrorsWithCopy(Function &F, RangeErrorMap &RMap,
					    FunctionCopyManager &FCMap,
					    SmallVectorImpl<Value *> *Args,
					    bool GenMetadata) {
  if (F.empty() || !propagateFunction(F))
    return;

  Function *CFP = FCMap.getFunctionCopy(&F);
  if (CFP == nullptr)
    return;

  // Increase count of consecutive recursive calls.
  unsigned OldRecCount = FCMap.incRecursionCount(&F);

  Function &CF = *CFP;

  DEBUG(dbgs() << "Processing function " << CF.getName()
	<< " (iteration " << OldRecCount + 1 << ")...\n");

  CmpErrorMap CmpMap(CMPERRORMAP_NUMINITBUCKETS);
  RangeErrorMap LocalRMap = RMap;
  // Reset the error associated to this function.
  LocalRMap.erase(CFP);

  CFLSteensAAWrapperPass *CFLSAA =
    this->getAnalysisIfAvailable<CFLSteensAAWrapperPass>();
  if (CFLSAA != nullptr)
    CFLSAA->getResult().scan(CFP);

  MemorySSA &MemSSA = this->getAnalysis<MemorySSAWrapperPass>(CF).getMSSA();

  computeErrors(CF, LocalRMap, CmpMap, FCMap, MemSSA, Args);

  if (GenMetadata) {
    // Put error metadata in original function.
    ValueToValueMapTy *VMap = FCMap.getValueToValueMap(&F);
    assert(VMap != nullptr);
    attachErrorMetadata(F, LocalRMap, CmpMap, *VMap);
  }

  // Associate computed errors to global variables.
  for (const GlobalVariable &GV : F.getParent()->globals()) {
    const AffineForm<inter_t> *GVErr = LocalRMap.getError(&GV);
    if (GVErr == nullptr)
      continue;
    RMap.setError(&GV, *GVErr);
  }

  // Associate computed error to the original function.
  auto FErr = LocalRMap.getError(CFP);
  if (FErr != nullptr)
    RMap.setError(&F, AffineForm<inter_t>(*FErr));

  // Restore original recursion count.
  FCMap.setRecursionCount(&F, OldRecCount);
}

void ErrorPropagator::computeErrors(Function &F, RangeErrorMap &RMap,
				    CmpErrorMap &CmpMap,
				    FunctionCopyManager &FCMap,
				    MemorySSA &MemSSA,
				    SmallVectorImpl<Value *> *ArgErrs) {
  RMap.retrieveRangeErrors(F);
  RMap.applyArgumentErrors(F, ArgErrs);

  // Compute errors for all instructions in the function
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    computeErrors(*I, RMap, CmpMap, FCMap, MemSSA);
  }
}

void ErrorPropagator::computeErrors(Instruction &I, RangeErrorMap &RMap,
				    CmpErrorMap &CmpMap,
				    FunctionCopyManager &FCMap,
				    MemorySSA &MemSSA) {
  RMap.retrieveRange(&I);

  dispatchInstruction(I, RMap, CmpMap, FCMap, MemSSA);
}

void ErrorPropagator::dispatchInstruction(Instruction &I,
					  RangeErrorMap &RMap,
					  CmpErrorMap &CmpMap,
					  FunctionCopyManager &FCMap,
					  MemorySSA &MemSSA) {
  if (I.isBinaryOp()) {
    propagateBinaryOp(RMap, I);
    return;
  }

  switch (I.getOpcode()) {
    case Instruction::Store:
      propagateStore(RMap, I);
      break;
    case Instruction::Load:
      propagateLoad(RMap, MemSSA, I);
      break;
    case Instruction::SExt:
      // Fall-through.
    case Instruction::ZExt:
      propagateIExt(RMap, I);
      break;
    case Instruction::Trunc:
      propagateTrunc(RMap, I);
      break;
    case Instruction::Select:
      propagateSelect(RMap, I);
      break;
    case Instruction::PHI:
      propagatePhi(RMap, I);
      break;
    case Instruction::ICmp:
      checkICmp(RMap, CmpMap, I);
      break;
    case Instruction::Ret:
      propagateRet(RMap, I);
      break;
    case Instruction::Call:
      prepareErrorsForCall(RMap, CmpMap, FCMap, I);
      propagateCall(RMap, I);
      break;
    // case Instruction::GetElementPtr:
    //   propagateGetElementPtr(RMap, I);
    //   break;
    default:
      DEBUG(dbgs() << "Unhandled " << I.getOpcodeName()
	    << " instruction: " << I.getName() << "\n");
      break;
  }
}

void ErrorPropagator::prepareErrorsForCall(RangeErrorMap &RMap,
					   CmpErrorMap &CmpMap,
					   FunctionCopyManager &FCMap,
					   Instruction &I) {
  CallInst &CI = cast<CallInst>(I);

  Function *CalledF = CI.getCalledFunction();
  if (CalledF == nullptr)
    return;

  DEBUG(dbgs() << "Preparing errors for function call " << CI.getName() << "...\n");

  SmallVector<Value *, 0U> Args(CI.arg_operands());

  // Stop if we have reached the maximum recursion count.
  if (FCMap.maxRecursionCountReached(CalledF))
    return;

  // Now propagate the errors for this call.
  computeErrorsWithCopy(*CalledF, RMap, FCMap, &Args, false);
}

void ErrorPropagator::attachErrorMetadata(Function &F,
					  const RangeErrorMap &RMap,
					  const CmpErrorMap &CmpMap,
					  ValueToValueMapTy &VMap) {
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Value *InstCopy = VMap[cast<Value>(&*I)];
    if (InstCopy == nullptr)
      continue;

    const AffineForm<inter_t> *Error = RMap.getError(InstCopy);
    if (Error != nullptr)
      setErrorMetadata(*I, *Error);

    CmpErrorMap::const_iterator CmpErr = CmpMap.find(InstCopy);
    if (CmpErr != CmpMap.end())
      setCmpErrorMetadata(*I, CmpErr->second);
  }
}

void ErrorPropagator::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<TargetLibraryInfoWrapperPass>();
  AU.addRequiredTransitive<MemorySSAWrapperPass>();
  AU.setPreservesAll();
}

void ErrorPropagator::checkCommandLine() {
  if (CmpErrorThreshold > 100U)
    CmpErrorThreshold = 100U;
}

}  // end of namespace ErrorProp

char ErrorProp::ErrorPropagator::ID = 0;


static RegisterPass<ErrorProp::ErrorPropagator>
X("errorprop", "Fixed-Point Arithmetic Error Propagator",
  false /* Only looks at CFG */,
  false /* Analysis Pass */);

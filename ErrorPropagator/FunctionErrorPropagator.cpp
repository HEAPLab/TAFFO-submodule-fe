//===-- FunctionErrorPropagator.cpp - Error Propagator ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Error propagator for fixed point computations in a single function.
///
//===----------------------------------------------------------------------===//

#include "ErrorPropagator/FunctionErrorPropagator.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"

#include "ErrorPropagator/Propagators.h"
//#include "ErrorPropagator/EPUtils/Metadata.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void
FunctionErrorPropagator::computeErrorsWithCopy(RangeErrorMap &GlobRMap,
					       SmallVectorImpl<Value *> *Args,
					       bool GenMetadata) {
  if (F.empty() || FCopy == nullptr) {
    DEBUG(dbgs() << "Function " << F.getName() << " could not be processed.\n");
    return;
  }

  // Increase count of consecutive recursive calls.
  unsigned OldRecCount = FCMap.incRecursionCount(&F);

  Function &CF = *FCopy;

  DEBUG(dbgs() << "Processing function " << CF.getName()
	<< " (iteration " << OldRecCount + 1 << ")...\n");

  CmpMap.clear();
  RMap = GlobRMap;
  // Reset the error associated to this function.
  RMap.erase(FCopy);

  CFLSteensAAWrapperPass *CFLSAA =
    EPPass.getAnalysisIfAvailable<CFLSteensAAWrapperPass>();
  if (CFLSAA != nullptr)
    CFLSAA->getResult().scan(FCopy);

  MemSSA = &(EPPass.getAnalysis<MemorySSAWrapperPass>(CF).getMSSA());

  computeFunctionErrors(Args);

  if (GenMetadata) {
    // Put error metadata in original function.
    attachErrorMetadata();
  }

  // Associate computed errors to global variables.
  for (const GlobalVariable &GV : F.getParent()->globals()) {
    const AffineForm<inter_t> *GVErr = RMap.getError(&GV);
    if (GVErr == nullptr)
      continue;
    GlobRMap.setError(&GV, *GVErr);
  }

  // Associate computed error to the original function.
  auto FErr = RMap.getError(FCopy);
  if (FErr != nullptr)
    GlobRMap.setError(&F, AffineForm<inter_t>(*FErr));

  // Restore original recursion count.
  FCMap.setRecursionCount(&F, OldRecCount);
}

void
FunctionErrorPropagator::computeFunctionErrors(SmallVectorImpl<Value *> *ArgErrs) {
  assert(FCopy != nullptr);

  RMap.retrieveRangeErrors(*FCopy);
  RMap.applyArgumentErrors(*FCopy, ArgErrs);

  // Compute errors for all instructions in the function
  BBScheduler BBSched(*FCopy);
  for (BasicBlock *BB : BBSched)
    for (Instruction &I : *BB)
      computeInstructionErrors(I);
}

void
FunctionErrorPropagator::computeInstructionErrors(Instruction &I) {
  bool HasInitialError = RMap.retrieveRangeError(I);

  double InitialError;
  if (HasInitialError) {
    auto *IEP = RMap.getError(&I);
    assert(IEP != nullptr);
    InitialError = IEP->noiseTermsAbsSum();
  }

  bool ComputedError = dispatchInstruction(I);

  if (HasInitialError && ComputedError) {
    DEBUG(dbgs() << "WARNING: computed error for instruction "
	  << I.getName() << " ignored because of metadata error "
	  << InitialError << ".\n");
    RMap.setError(&I, AffineForm<inter_t>(0.0, InitialError));
  }
}

bool
FunctionErrorPropagator::dispatchInstruction(Instruction &I) {
  assert(MemSSA != nullptr);

  if (I.isBinaryOp())
    return propagateBinaryOp(RMap, I);

  switch (I.getOpcode()) {
    case Instruction::Store:
      return propagateStore(RMap, I);
    case Instruction::Load:
      return propagateLoad(RMap, *MemSSA, I);
    case Instruction::SExt:
      // Fall-through.
    case Instruction::ZExt:
      return propagateIExt(RMap, I);
    case Instruction::Trunc:
      return propagateTrunc(RMap, I);
    case Instruction::Select:
      return propagateSelect(RMap, I);
    case Instruction::PHI:
      return propagatePhi(RMap, I);
    case Instruction::ICmp:
      return checkICmp(RMap, CmpMap, I);
    case Instruction::Ret:
      return propagateRet(RMap, I);
    case Instruction::Call:
      prepareErrorsForCall(I);
      return propagateCall(RMap, I);
    case Instruction::Invoke:
      prepareErrorsForCall(I);
      return propagateCall(RMap, I);
    default:
      DEBUG(dbgs() << "Unhandled " << I.getOpcodeName()
	    << " instruction: " << I.getName() << "\n");
  }
}

void
FunctionErrorPropagator::prepareErrorsForCall(Instruction &I) {
  Function *CalledF = nullptr;
  SmallVector<Value *, 0U> Args;
  if (isa<CallInst>(I)) {
    CallInst &CI = cast<CallInst>(I);
    CalledF = CI.getCalledFunction();
    Args.append(CI.arg_begin(), CI.arg_end());
  }
  else if (isa<InvokeInst>(I)) {
    InvokeInst &II = cast<InvokeInst>(I);
    CalledF = II.getCalledFunction();
    Args.append(II.arg_begin(), II.arg_end());
  }

  if (CalledF == nullptr)
    return;

  DEBUG(dbgs() << "Preparing errors for function call/invoke "
	<< I.getName() << "...\n");

  // Stop if we have reached the maximum recursion count.
  if (FCMap.maxRecursionCountReached(CalledF))
    return;

  // Now propagate the errors for this call.
  FunctionErrorPropagator CFEP(EPPass, *CalledF,
			       FCMap, RMap.getMetadataManager());
  CFEP.computeErrorsWithCopy(RMap, &Args, false);

  // Restore MemorySSA
  EPPass.getAnalysis<MemorySSAWrapperPass>(*I.getFunction());
}

void
FunctionErrorPropagator::attachErrorMetadata() {
  ValueToValueMapTy *VMap = FCMap.getValueToValueMap(&F);
  assert(VMap != nullptr);

  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Value *InstCopy = (Cloned) ? (*VMap)[cast<Value>(&*I)] : &*I;
    if (InstCopy == nullptr)
      continue;

    const AffineForm<inter_t> *Error = RMap.getError(InstCopy);
    if (Error != nullptr)
      MetadataManager::setErrorMetadata(*I, Error->noiseTermsAbsSum());

    CmpErrorMap::const_iterator CmpErr = CmpMap.find(InstCopy);
    if (CmpErr != CmpMap.end())
      MetadataManager::setCmpErrorMetadata(*I, CmpErr->second);
  }
}

void BBScheduler::enqueueChildren(BasicBlock *BB) {
  assert(BB != nullptr && "Null basic block.");

  DEBUG(dbgs() << "Scheduling " << BB->getName() << ".\n");

  Set.insert(BB);

  TerminatorInst *TI = BB->getTerminator();
  if (TI != nullptr) {
    for (BasicBlock *DestBB : TI->successors())
      if (!Set.count(DestBB))
	enqueueChildren(DestBB);
      else if (hasUnvisitedChildren(DestBB))
	enqueueUnvisitedChildren(DestBB);
  }
  Queue.push_back(BB);
}

void BBScheduler::enqueueUnvisitedChildren(BasicBlock *BB) {
  assert(BB != nullptr && "Null basic block.");

  DEBUG(dbgs() << "Scheduling " << BB->getName() << ".\n");

  TerminatorInst *TI = BB->getTerminator();
  if (TI != nullptr) {
    for (BasicBlock *DestBB : TI->successors())
      if (!Set.count(DestBB))
	enqueueChildren(DestBB);
  }
  // Queue.push_back(BB);
}

bool BBScheduler::hasUnvisitedChildren(BasicBlock *BB) const {
  TerminatorInst *TI = BB->getTerminator();
  if (TI != nullptr) {
    for (BasicBlock *DestBB : TI->successors())
      if (!Set.count(DestBB))
	return true;
  }
  return false;
}

} // end namespace ErrorProp

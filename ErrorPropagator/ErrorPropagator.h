//===-- ErrorPropagator.h - Error Propagator --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This LLVM opt pass propagates errors in fixed point computations.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_H
#define ERRORPROPAGATOR_H

#include "llvm/Support/CommandLine.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/ADT/SmallVector.h"

#include "RangeErrorMap.h"
#include "FunctionCopyMap.h"

namespace ErrorProp {

using namespace llvm;

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

} // end namespace ErrorProp

#endif

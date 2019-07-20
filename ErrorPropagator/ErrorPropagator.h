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

#include "RangeErrorMap.h"
#include "FunctionCopyMap.h"

namespace ErrorProp {

llvm::cl::opt<unsigned> DefaultUnrollCount("dunroll",
					   llvm::cl::desc("Default loop unroll count"),
					   llvm::cl::value_desc("count"),
					   llvm::cl::init(1U));
llvm::cl::opt<bool> NoLoopUnroll("nounroll",
				 llvm::cl::desc("Never unroll loops"),
				 llvm::cl::init(false));
llvm::cl::opt<unsigned> CmpErrorThreshold("cmpthresh",
					  llvm::cl::desc("CMP errors are signaled"
							 "only if error is above perc %"),
					  llvm::cl::value_desc("perc"),
					  llvm::cl::init(0U));
llvm::cl::opt<unsigned> MaxRecursionCount("recur",
					  llvm::cl::desc("Default number of recursive calls"
							 "to the same function."),
					  llvm::cl::value_desc("count"),
					  llvm::cl::init(1U));
llvm::cl::opt<bool> StartOnly("startonly",
			      llvm::cl::desc("Propagate only functions with start metadata."),
			      llvm::cl::init(false));
llvm::cl::opt<bool> Absolute("abserror",
			      llvm::cl::desc("Output absolute errors instead of relative errors."),
			      llvm::cl::init(false));

class ErrorPropagator : public llvm::ModulePass {
public:
  static char ID;
  ErrorPropagator() : ModulePass(ID) {}

  bool runOnModule(llvm::Module &) override;
  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

protected:
  void retrieveGlobalVariablesRangeError(llvm::Module &M, RangeErrorMap &RMap);
  void checkCommandLine();

}; // end of class ErrorPropagator

} // end namespace ErrorProp

#endif

//===-- ErrorPropagator.cpp - Error Propagator ------------------*- C++ -*-===//
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

#include "ErrorPropagator.h"

#include <cassert>
#include <utility>

#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"

#include "llvm/Transforms/ErrorPropagator/AffineForms.h"
#include "llvm/Transforms/ErrorPropagator/Metadata.h"
#include "FunctionErrorPropagator.h"
#include "Propagators.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

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
    FunctionErrorPropagator FEP(*this, *F, FCMap);
    FEP.computeErrorsWithCopy(GlobalRMap, nullptr, true);
  }

  return false;
}

void ErrorPropagator::retrieveGlobalVariablesRangeError(const Module &M,
							RangeErrorMap &RMap) {
  for (const GlobalVariable &GV : M.globals()) {
    RMap.retrieveRangeError(GV);
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

static llvm::RegisterPass<ErrorProp::ErrorPropagator>
X("errorprop", "Fixed-Point Arithmetic Error Propagator",
  false /* Only looks at CFG */,
  false /* Analysis Pass */);

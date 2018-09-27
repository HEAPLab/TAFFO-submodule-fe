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

#include "ErrorPropagator/ErrorPropagator.h"

#include <cassert>
#include <utility>

#include "llvm/Support/Debug.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/MemorySSA.h"

#include "Metadata.h"
#include "ErrorPropagator/FunctionErrorPropagator.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

bool ErrorPropagator::runOnModule(Module &M) {
  checkCommandLine();

  MetadataManager &MDManager = MetadataManager::getMetadataManager();

  RangeErrorMap GlobalRMap(MDManager);

  // Get Ranges and initial Errors for global variables.
  retrieveGlobalVariablesRangeError(M, GlobalRMap);

  // Copy list of original functions, so we don't mess up with copies.
  SmallVector<Function *, 4U> Functions;
  Functions.reserve(M.size());
  for (Function &F : M) {
    Functions.push_back(&F);
  }

  FunctionCopyManager FCMap(*this, MaxRecursionCount, DefaultUnrollCount,
			    NoLoopUnroll);

  // Iterate over all functions in this Module,
  // and propagate errors for pending input intervals for all of them.
  for (Function *F : Functions) {
    if (StartOnly && !MetadataManager::isStartingPoint(*F))
      continue;

    FunctionErrorPropagator FEP(*this, *F, FCMap, MDManager);
    FEP.computeErrorsWithCopy(GlobalRMap, nullptr, true);
  }

  dbgs() << "\n*** Target Errors: ***\n";
  GlobalRMap.printTargetErrors(dbgs());

  return false;
}

void ErrorPropagator::retrieveGlobalVariablesRangeError(const Module &M,
							RangeErrorMap &RMap) {
  for (const GlobalVariable &GV : M.globals()) {
    RMap.retrieveRangeError(GV);
  }
}

void ErrorPropagator::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<LoopInfoWrapperPass>();
  AU.addRequiredTransitive<AssumptionCacheTracker>();
  AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
  AU.addRequiredTransitive<OptimizationRemarkEmitterWrapperPass>();
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

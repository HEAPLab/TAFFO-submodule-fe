#include "FunctionCopyMap.h"

#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/ErrorPropagator/Metadata.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void UnrollLoops(Pass &P, Function &F, unsigned DefaultUnrollCount) {
  // Prepare required analyses
  DominatorTree &DomTree =
    P.getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
  LoopInfo &LInfo =
    P.getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
  AssumptionCache &AssC =
    P.getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
  ScalarEvolution &SE =
    P.getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
  OptimizationRemarkEmitter &ORE =
    P.getAnalysis<OptimizationRemarkEmitterWrapperPass>(F).getORE();

  // Now try to unroll all loops
  for (Loop *L : LInfo) {
    // Compute loop trip count
    unsigned TripCount = SE.getSmallConstantTripCount(L);
    // Get user supplied unroll count
    Optional<unsigned> OUC = retrieveLoopUnrollCount(*L);
    unsigned UnrollCount = DefaultUnrollCount;
    if (OUC.hasValue())
      if (TripCount != 0 && OUC.getValue() > TripCount)
	UnrollCount = TripCount;
      else
	UnrollCount = OUC.getValue();
    else if (TripCount != 0)
      UnrollCount = TripCount;

    DEBUG(dbgs() << "Trying to unroll loop by " << UnrollCount << "... ");

    // Actually unroll loop
    LoopUnrollResult URes = UnrollLoop(L, UnrollCount, TripCount,
    				       true, false, true, false, false,
    				       SE.getSmallConstantTripMultiple(L), 0U,
    				       false, &LInfo, &SE, &DomTree,
    				       &AssC, &ORE, true);
    switch (URes) {
      case LoopUnrollResult::Unmodified:
    	DEBUG(dbgs() << "unmodified.\n");
    	break;
      case LoopUnrollResult::PartiallyUnrolled:
    	DEBUG(dbgs() << "unrolled partially.\n");
    	break;
      case LoopUnrollResult::FullyUnrolled:
    	DEBUG(dbgs() << "done.\n");
    	break;
    }
  }
}

FunctionCopyCount *FunctionCopyManager::prepareFunctionData(Function *F) {
  auto FCData = FCMap.find(F);
  if (FCData == FCMap.end()) {
    // If no copy of F has already been made, create one, so loop transformations
    // do not change original code.
    FunctionCopyCount &FCC = FCMap[F];

    if ((FCC.MaxRecCount = retrieveMaxRecursionCount(*F)) == 0U)
      FCC.MaxRecCount = MaxRecursionCount;

    if (!F->empty())
      FCC.Copy = CloneFunction(F, FCC.VMap);

    if (FCC.Copy != nullptr && !NoLoopUnroll)
      UnrollLoops(P, *FCC.Copy, DefaultUnrollCount);

    return &FCC;
  }
  return &FCData->second;
}

FunctionCopyManager::~FunctionCopyManager() {
  for (auto &FCC : FCMap) {
    if (FCC.second.Copy != nullptr)
      FCC.second.Copy->eraseFromParent();
  }
}

} // end namespace ErrorProp

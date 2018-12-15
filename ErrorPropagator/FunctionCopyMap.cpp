#include "ErrorPropagator/FunctionCopyMap.h"

#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/OptimizationDiagnosticInfo.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Support/Debug.h"
#include "Metadata.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void UnrollLoops(Pass &P, Function &F, unsigned DefaultUnrollCount) {
  // Prepare required analyses
  LoopInfo &LInfo = P.getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

  // Now try to unroll all loops
  for (Loop *L : LInfo) {
    ScalarEvolution &SE = P.getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
    // Compute loop trip count
    unsigned TripCount = SE.getSmallConstantTripCount(L);
    // Get user supplied unroll count
    Optional<unsigned> OUC = mdutils::MetadataManager::retrieveLoopUnrollCount(*L);
    unsigned UnrollCount = DefaultUnrollCount;
    if (OUC.hasValue())
      if (TripCount != 0 && OUC.getValue() > TripCount)
	UnrollCount = TripCount;
      else
	UnrollCount = OUC.getValue();
    else if (TripCount != 0)
      UnrollCount = TripCount;

    DEBUG(dbgs() << "Trying to unroll loop by " << UnrollCount << "... ");

    unsigned TripMult = SE.getSmallConstantTripMultiple(L);
    if (TripMult == 0U)
      TripMult = UnrollCount;

    // Actually unroll loop
    DominatorTree &DomTree = P.getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
    AssumptionCache &AssC = P.getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    OptimizationRemarkEmitter &ORE = P.getAnalysis<OptimizationRemarkEmitterWrapperPass>(F).getORE();
    
    bool URes = UnrollLoop(L, UnrollCount, TripCount,
    				       true, false, true, false, false,
    				       TripMult, 0U,
    				       &LInfo, &SE, &DomTree,
    				       &AssC, &ORE, true);
    switch (URes) {
      case false:
    	DEBUG(dbgs() << "unmodified.\n");
    	break;
      case true:
    	DEBUG(dbgs() << "done.\n");
    	break;
    }
  }
}

FunctionCopyCount *FunctionCopyManager::prepareFunctionData(Function *F) {
  assert(F != nullptr);

  auto FCData = FCMap.find(F);
  if (FCData == FCMap.end()) {
    // If no copy of F has already been made, create one, so loop transformations
    // do not change original code.
    FunctionCopyCount &FCC = FCMap[F];

    if ((FCC.MaxRecCount = mdutils::MetadataManager::retrieveMaxRecursionCount(*F)) == 0U)
      FCC.MaxRecCount = MaxRecursionCount;

    // Check if we really need to clone the function
    if (!NoLoopUnroll && !F->empty()) {
      LoopInfo &LInfo =
	P.getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
      if (!LInfo.empty()) {
	FCC.Copy = CloneFunction(F, FCC.VMap);

	if (FCC.Copy != nullptr)
	  UnrollLoops(P, *FCC.Copy, DefaultUnrollCount);
      }
    }
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

#include "FunctionCopyMap.h"

#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Support/Debug.h"


namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void UnrollLoops(Function &F, unsigned DefaultUnrollCount,
		 TargetLibraryInfo &TLI) {
  // Prepare LoopInfo
  DominatorTree DomTree(F);
  LoopInfo LInfo(DomTree);

  // Prepare ScalarEvolution
  AssumptionCache AssC(F);
  ScalarEvolution SE(F, TLI, AssC, DomTree, LInfo);
  OptimizationRemarkEmitter ORE(&F);

  // Now try to unroll all loops
  for (Loop *L : LInfo) {
    // Compute loop trip count
    unsigned TripCount = SE.getSmallConstantTripCount(L);
    // If no trip count can be computed, unroll by default amount
    unsigned UnrollCount = (TripCount == 0) ? DefaultUnrollCount : TripCount;

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
    FCC.Copy = CloneFunction(F, FCC.VMap);

    if (FCC.Copy != nullptr && !NoLoopUnroll)
      UnrollLoops(*FCC.Copy, DefaultUnrollCount, TLI);

    return &FCC;
  }
  return &FCData->second;
}

FunctionCopyManager::~FunctionCopyManager() {
  for (auto &FCC : FCMap) {
    FCC.second.Copy->eraseFromParent();
  }
}

} // end namespace ErrorProp

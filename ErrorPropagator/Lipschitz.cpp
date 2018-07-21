#include "ErrorPropagator/Lipschitz.h"

#include "llvm/Support/Debug.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void LipschitzLoopPropagator::populateLoopStructure(DominatorTree& DT) {
  if (L.isInvalid()) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (invalid).\n");
    IsValid = false;
    return;
  }

  if (!L.isLoopSimplifyForm()) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (not in simplified form).\n");
    IsValid = false;
    return;
  }

  if (!L.isLCSSAForm(DT)) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (not in LCSSA form).\n");
    IsValid = false;
    return;
  }

  if (!L.getSubLoops().empty()) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (contains inner loops).\n");
    IsValid = false;
    return;
  }

  if (L.getNumBlocks() > 2)  {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (CFG too complex).\n");
    IsValid = false;
    return;
  }

  S.Predecessor = L.getLoopPredecessor();
  if (S.Predecessor == nullptr) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (no single predecessor).\n");
    IsValid = false;
    return;
  }

  S.Exit = L.getExitBlock();
  if (S.Exit == nullptr) {
    DEBUG(dbgs() << "Lipschitz: ignoring loop (no single exit block).\n");
    IsValid = false;
    return;
  }

  if (L.getNumBlocks() == 1) {
    S.Body = S.Cond = L.getHeader();
  }
  else {
    S.Cond = L.getExitingBlock();
    if (S.Cond == nullptr) {
      DEBUG(dbgs() << "Lipschitz: ignoring loop (no single exiting block).\n");
      IsValid = false;
      return;
    }

    S.Body = L.getLoopLatch();
    if (S.Body == nullptr) {
      DEBUG(dbgs() << "Lipschitz: ignoring loop (no single latch).\n");
      IsValid = false;
      return;
    }
    if (S.Body == S.Exit) {
      // The latch starts from the condition block,
      // so we consider the header as body.
      S.Body = L.getHeader();
    }
  }

  assert(S.check());
}

void LipschitzLoopPropagator::reconstructExitingExpressions
(SmallVectorImpl<std::unique_ptr<EPExpr> > &Exprs) const {
  for (PHINode &PHI : S.Exit->phis()) {
    DEBUG(dbgs() << "Reconstructing expression for PHINode "
	  << PHI.getName() << ": ");

    std::unique_ptr<EPExpr> Expr = EPExpr::BuildEPExprFromLCSSA(S, PHI);

    if (Expr == nullptr) {
      DEBUG(dbgs() << "failed.\n");
      continue;
    }

    DEBUG(dbgs() << Expr->toString() << ".\n");

    Exprs.push_back(std::move(Expr));
  }
}

void LipschitzLoopPropagator::computeErrors(unsigned TripCount) {
  SmallVector<std::unique_ptr<EPExpr>, 8U> Exprs;
  reconstructExitingExpressions(Exprs);

  dbgs() << "End.\n";
}

} // end namespace ErrorProp

#include "ErrorPropagator/Lipschitz.h"

#include "ErrorPropagator/IntervalVisitor.h"
#include "llvm/Support/Debug.h"
#include <sstream>
#include <ginac/ginac.h>

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

void LipschitzLoopPropagator::reconstructLoopExpressions
(DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) const {
  for (PHINode &PHI : S.Cond->phis()) {
    DEBUG(dbgs() << "Reconstructing expression for PHINode "
	  << PHI.getName() << "...\n");

    EPExpr::BuildEPExprFromLCSSA(S, PHI, Exprs);
  }

  DEBUG(
	for (auto &Expr : Exprs) {
	  // TODO: remove assertion (just abort)
	  assert(Expr.second != nullptr);
	  dbgs() << Expr.first->getName() << " = "
		 << Expr.second->toString() << "\n";
	}
	);
}

void LipschitzLoopPropagator::computeErrors(unsigned TripCount) {
  // Reconstruct expressions of each loop variable,
  // keeping track of their initial values
  DenseMap<Value *, std::unique_ptr<EPExpr> > Exprs;
  reconstructLoopExpressions(Exprs);

  // We need these two maps to associate GiNaC symbols to IR Values
  MapVector<Value *, GiNaC::symbol> ValToSym;
  std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> SymToVal;
  makeSymbols(Exprs, ValToSym, SymToVal);
  assert(ValToSym.size() == SymToVal.size());

  // Convert our EPExprs into GiNaC expressions
  DenseMap<Value *, GiNaC::ex> SymExprs;
  makeSymExprs(Exprs, ValToSym, SymExprs);

  // Compute the Lipschitz matrix K
  GiNaC::matrix Lipsc(ValToSym.size(), ValToSym.size());
  buildLipschitzMatrix(ValToSym, SymToVal, SymExprs, Lipsc);
  DEBUG(dbgs() << "Lipschitz matrix K:\n"
	<< GiNaCexToString(Lipsc) << "\n");

  // Compute K^k, where k = TripCount, the matrix to be multiplied to initial errors
  GiNaC::matrix LipscK = Lipsc.pow(TripCount);
  DEBUG(dbgs() << "K^k:\n" << GiNaCexToString(LipscK) << "\n");

  // Compute the matrix to be multiplied by the roundoff error
  GiNaC::matrix RoundErrM = computeRoundoffErrorMatrix(Lipsc, LipscK, TripCount);

  // Retrieve initial errors
  GiNaC::matrix InitErrs = getInitialErrors(ValToSym, Exprs);

  // Retrieve roundoff errors
  // TODO!

  // Compute error vector
  GiNaC::matrix Errors = LipscK.mul(InitErrs);

  // Associate errors to exiting values
  // TODO
}

void
LipschitzLoopPropagator::makeSymbols(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
				     MapVector<Value *, GiNaC::symbol> &ValToSym,
				     std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal) {
  for (auto &Expr : Exprs) {
    GiNaC::symbol Var;
    Var.set_name(Var.get_name() + "_" + Expr.first->getName().str());
    ValToSym.insert(std::make_pair(Expr.first, Var));
    SymToVal.insert(std::make_pair(Var, Expr.first));
  }
}

void
LipschitzLoopPropagator::makeSymExprs(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
				      const MapVector<Value *, GiNaC::symbol> &ValToSym,
				      DenseMap<Value *, GiNaC::ex> &SymExprs) {
  DEBUG(dbgs() << "Converting to GiNaC expressions...\n");
  for (auto &Expr : Exprs)
    SymExprs.insert(std::make_pair(Expr.first, Expr.second->toSymbolicEx(ValToSym)));

  DEBUG(
  for (auto &VEx : SymExprs) {
    dbgs() << VEx.first->getName().str() << " = "
	   << GiNaCexToString(VEx.second) << "\n";
  }
	);
}

void LipschitzLoopPropagator::
buildLipschitzMatrix(const MapVector<Value *, GiNaC::symbol> &ValToSym,
		     const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal,
		     const DenseMap<Value *, GiNaC::ex> &SymExprs,
		     GiNaC::matrix &Res) {
  assert(Res.rows() == ValToSym.size()
	 && Res.cols() == ValToSym.size()
	 && "Must be a square matrix whose size is the number of variables.");
  DEBUG(dbgs() << "Computing derivatives and Lipschitz matrix...\n");

  unsigned r = 0U;
  for (auto &VarR : ValToSym) {
    auto VExpr = SymExprs.find(VarR.first);
    assert(VExpr != SymExprs.end());
    const GiNaC::ex &Fun = VExpr->second;

    unsigned c = 0U;
    for (auto &VarC : ValToSym) {
      GiNaC::ex dFun = Fun.diff(VarC.second);
      Res(r, c) = evaluateExpression(SymToVal, dFun);

      DEBUG(dbgs() << "d " << VarR.first->getName().str()
	    << " / d " << VarC.first->getName().str()
	    << " = " << GiNaCexToString(dFun)
	    << " = " << GiNaCexToString(Res(r, c)) << "\n");

      ++c;
    }
    ++r;
  }
}

GiNaC::ex LipschitzLoopPropagator::
evaluateExpression(const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal,
		   const GiNaC::ex &Expr) {
  IntervalVisitor Visitor(RMap, SymToVal);
  Expr.traverse_postorder(Visitor);
  return static_cast<double>(Visitor.getMaxAbsValue());
}

GiNaC::matrix LipschitzLoopPropagator::
computeRoundoffErrorMatrix(const GiNaC::matrix &K, const GiNaC::matrix &Kk, unsigned k) {
  DEBUG(dbgs() << "Computing roundoff error matrix ((I-K)^-1 (I-K^k))...");
  GiNaC::ex Unit = unit_matrix(K.rows());
  try {
    // First try with the fast method
    GiNaC::ex ErrM = (pow(Unit - K, -1) * (Unit - Kk)).evalm();
    DEBUG(dbgs() << "done:\n" << GiNaCexToString(ErrM) << "\n");
    return ex_to<GiNaC::matrix>(ErrM);

  } catch (const std::runtime_error &E) {
    DEBUG(dbgs() << " falling back to slow method... ");
    GiNaC::matrix Ki = ex_to<GiNaC::matrix>(Unit);
    GiNaC::matrix Res(K.rows(), K.cols());
    for (unsigned i = 0; i < k; ++i) {
      Res = Res.add(Ki);
      Ki = Ki.mul(K);
    }

    DEBUG(dbgs() << "done:\n" << GiNaCexToString(Res) << "\n");
    return std::move(Res);
  }
}

GiNaC::matrix LipschitzLoopPropagator::
getInitialErrors(MapVector<Value *, GiNaC::symbol> &ValToSym,
		 DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) {
  DenseMap<Value *, Value *> InitialValues(ValToSym.size());
  for (auto &VExpr : Exprs) {
    EPExpr *Expr = VExpr.second.get();
    if (isa<EPLeaf>(Expr)) {
      EPLeaf *Leaf = cast<EPLeaf>(Expr);
      Value *Ext = Leaf->getExternal();
      if (Ext != nullptr)
	InitialValues.insert(std::make_pair(VExpr.first, Ext));
    }
  }

  for (PHINode &PHI : S.Cond->phis()) {
    Value *BV = PHI.getIncomingValueForBlock(S.Body);
    Value *EV = PHI.getIncomingValueForBlock(S.Predecessor);
    if (BV != nullptr && EV != nullptr)
      InitialValues.insert(std::make_pair(BV, EV));
  }

  GiNaC::matrix InitialErrors(ValToSym.size(), 1);
  unsigned i = 0U;
  for (auto &ValSym : ValToSym) {
    auto InitVal = InitialValues.find(ValSym.first);
    if (InitVal != InitialValues.end()) {
      const AffineForm<inter_t> *Err = RMap.getError(InitVal->second);
      if (Err != nullptr) {
	InitialErrors(i, 0) = static_cast<double>(Err->noiseTermsAbsSum());
	continue;
      }
    }
      InitialErrors(i, 0) = 0;
      DEBUG(dbgs() << "Initial error for " << ValSym.first->getName() << " not found.\n");
  }

  return std::move(InitialErrors);
}

} // end namespace ErrorProp

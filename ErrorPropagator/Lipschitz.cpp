#include "ErrorPropagator/Lipschitz.h"

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

  for (auto &Expr : Exprs) {
    DEBUG(dbgs() << Expr.first->getName() << " = "
	  << Expr.second->toString() << "\n");
  }
}

void LipschitzLoopPropagator::computeErrors(unsigned TripCount) {
  DenseMap<Value *, std::unique_ptr<EPExpr> > Exprs;
  reconstructLoopExpressions(Exprs);

  MapVector<Value *, GiNaC::symbol> ValToSym;
  std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> SymToVal;
  makeSymbols(Exprs, ValToSym, SymToVal);

  DenseMap<Value *, GiNaC::ex> SymExprs;
  makeSymExprs(Exprs, Symbols, SymExprs);
}

void
LipschitzLoopPropagator::makeSymbols(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
				     MapVector<Value *, GiNaC::symbol> &ValToSym,
				     std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal) {
  for (auto &Expr : Exprs) {
    GiNaC::symbol Var;
    Var.set_name(Var.get_name() + Expr.first->getName().str());
    ValToSym.insert(std::make_pair(Expr.first, Var));
    SymToVal.insert(std::make_pair(Var, Expr.first));
  }
}

void
LipschitzLoopPropagator::makeSymExprs(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
				      const MapVector<Value *, GiNaC::symbol> &ValToSym,
				      DenseMap<Value *, GiNaC::ex> &SymExprs) {
  for (auto &Expr : Exprs)
    SymExprs.insert(std::make_pair(Expr.first, Expr.second->toSymbolicEx(ValToSym)));

  DEBUG(
  for (auto &VEx : SymExprs) {
    std::ostringstream Str;
    Str << VEx.first->getName().str() << " = ";
    VEx.second.print(GiNaC::print_context(Str));
    Str << "\n";
    dbgs() << Str.str();
  }
	);
}

void LipschitzLoopPropagator::
buildLipschitzMatrix(const MapVector<Value *, GiNaC::symbol> &ValToSym,
		     const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal,
		     const DenseMap<Value *, GiNaC::ex> &SymExprs,
		     GiNaC::matrix &Res) {
  assert(Res.rows() == Symbols.size()
	 && Res.cols() == Symbols.size()
	 && "Must be a square matrix whose size is the number of variables.");

  unsigned r = 0U;
  unsigned c = 0U;
  for (auto &VarR : ValToSym) {
    auto VExpr = SymExprs.find(VarR.first);
    assert(VExpr != SymExprs.end());
    GiNaC::ex &Fun = VExpr.second;

    for (auto &VarC : ValToSym) {
      GiNaC::ex dFun = Fun.diff(VarC.second);
      Res(r, c) = computeLipschitzCoeff(RMap, SymToVal, Fun);
      ++c;
    }
    ++r;
  }
}

} // end namespace ErrorProp

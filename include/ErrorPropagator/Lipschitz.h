
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/MapVector.h"
#include "ErrorPropagator/RangeErrorMap.h"
#include "ErrorPropagator/EPExpr.h"

namespace ErrorProp {

using namespace llvm;

struct LipschitzLoopStructure {
  BasicBlock *Predecessor = nullptr;
  BasicBlock *Body = nullptr;
  BasicBlock *Cond = nullptr;
  BasicBlock *Exit = nullptr;

  bool check() const {
    return Predecessor && Body && Cond && Exit;
  }
};

class LipschitzLoopPropagator {
public:
  LipschitzLoopPropagator(const Loop &L, RangeErrorMap &RMap, DominatorTree &DT)
    : L(L), RMap(RMap), IsValid(true), S() {
    populateLoopStructure(DT);
  }

  bool isValid() {
    return IsValid && S.check();
  }

  void computeErrors(unsigned TripCount);

protected:
  const Loop &L;
  RangeErrorMap &RMap;

  bool IsValid;
  LipschitzLoopStructure S;

  void populateLoopStructure(DominatorTree& DT);

  void reconstructLoopExpressions
  (DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) const;

  void makeSymbols(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
		   MapVector<Value *, GiNaC::symbol> &ValToSym,
		   std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal);

  void makeSymExprs(const DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
		    const MapVector<Value *, GiNaC::symbol> &ValToSym,
		    DenseMap<Value *, GiNaC::ex> &SymExprs);

  void buildLipschitzMatrix(const MapVector<Value *, GiNaC::symbol> &ValToSym,
			    const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal,
			    const DenseMap<Value *, GiNaC::ex> &SymExprs,
			    GiNaC::matrix &Res);

  GiNaC::ex
  evaluateExpression(const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal,
		     const GiNaC::ex &Expr);

  GiNaC::matrix
  computeRoundoffErrorMatrix(const GiNaC::matrix &K, const GiNaC::matrix &Kk, unsigned k);

  GiNaC::matrix
  getInitialErrors(MapVector<Value *, GiNaC::symbol> &ValToSym,
		   DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs);
};

} // end namespace ErrorProp

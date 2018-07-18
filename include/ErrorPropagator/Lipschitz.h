
struct LipschitzLoopStructure {
  BasicBlock *Predecessor = nullptr;
  BasicBlock *Body = nullptr;
  BasicBlock *Cond = nullptr;
  BasicBlock *Exit = nullptr;

  bool check() const {
    return Predecessor && Body && Cond && Exit;
  }
};

namespace ErrorProp {

class LipschitzLoopPropagator {
public:
  LipschitzLoopPropagator(const Loop &L, RangeErrorMap &RMap, DominatorTree &DT)
    : L(L), RMap(RMap), IsValid(true), S() {
    populateLoopStructure();
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

  void populateLoopStructure();

  void reconstructExitingExpressions(SmallVectorImpl<EPExpr> &Exprs) const;
};

} // end namespace ErrorProp

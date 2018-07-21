
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/ADT/SmallVector.h"
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

  void reconstructExitingExpressions
  (DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) const;
};

} // end namespace ErrorProp

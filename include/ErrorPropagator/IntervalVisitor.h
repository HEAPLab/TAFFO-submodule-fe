#include "ErrorPropagator/RangeErrorMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include <map>
#include <boost/numeric/interval.hpp>
#include <ginac/ginac.h>

namespace ErrorProp {

using namespace llvm;
using namespace boost::numeric;
using namespace GiNaC;

class IntervalVisitor
  : public visitor,
    public add::visitor,
    public mul::visitor,
    public power::visitor,
    public numeric::visitor,
    public symbol::visitor,
    public basic::visitor {
public:
  IntervalVisitor(RangeErrorMap &RMap,
		  const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal)
    : RMap(RMap), SymToVal(SymToVal), Stack(), Valid(true) {}

  void visit(const add &x);
  void visit(const mul &x);
  void visit(const power &x);
  void visit(const numeric &x);
  void visit(const symbol &x);
  void visit(const basic &x);

  bool isValid() const { return Valid; }

  inter_t getMaxAbsValue() const;

protected:
  RangeErrorMap &RMap;
  const std::map<GiNaC::symbol, Value *, GiNaC::ex_is_less> &SymToVal;

  SmallVector<interval<inter_t>, 8U> Stack;
  bool Valid; // TODO remove if useless
};

std::string GiNaCexToString(const ex &x);

} // end namespace ErrorProp

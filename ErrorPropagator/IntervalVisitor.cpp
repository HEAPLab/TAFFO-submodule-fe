#include "ErrorPropagator/IntervalVisitor.h"

#include "llvm/Support/Debug.h"
#include <sstream>

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

void IntervalVisitor::visit(const add &x) {
  assert(x.nops() > 0 && "Must have at least one operand.");
  assert(Stack.size() >= x.nops() && "Must have processed all arguments.");

  interval<inter_t> Sum;
  for (std::size_t i = 0; i < x.nops(); ++i)
    Sum += Stack.pop_back_val();

  Stack.push_back(Sum);
}

void IntervalVisitor::visit(const mul &x) {
  assert(x.nops() > 0 && "Must have at least one operand.");
  assert(Stack.size() >= x.nops() && "Must have processed all arguments.");

  interval<inter_t> Prod(static_cast<inter_t>(1));
  for (std::size_t i = 0; i < x.nops(); ++i)
    Prod *= Stack.pop_back_val();

  Stack.push_back(Prod);
}

void IntervalVisitor::visit(const power &x) {
  assert(x.nops() == 2 && "Must have basis and exponent.");
  assert(Stack.size() >= x.nops() && "Must have processed all arguments.");

  interval<inter_t> Exp = Stack.pop_back_val();
  assert(singleton(Exp) && "Must be a singleton.");

  interval<inter_t> Basis = Stack.pop_back_val();

  Stack.push_back(pow(Basis, static_cast<int>(Exp.upper())));
}

void IntervalVisitor::visit(const numeric &x) {
  Stack.push_back(interval<inter_t>(static_cast<inter_t>(x.to_double())));
}

void IntervalVisitor::visit(const symbol &x) {
  auto SV = SymToVal.find(x);
  assert(SV != SymToVal.end()
	 && "Symbol x must have been associated to a Value.");

  Value *V = SV->second;
  if (isa<Instruction>(V))
    RMap.retrieveRange(cast<Instruction>(V));

  const FPInterval *Range = RMap.getRange(V);
  assert(Range != nullptr && "Value with unknown range.");

  Stack.push_back(interval<inter_t>(Range->Min, Range->Max));
}

void IntervalVisitor::visit(const basic &x) {
  DEBUG(dbgs() << "GiNaC subexpression not processed: "
	<< GiNaCexToString(x) << "\n");

  llvm_unreachable("Unexpected GiNaC subexpression type.");
}

inter_t IntervalVisitor::getMaxAbsValue() const {
  assert(Stack.size() == 1U);
  return norm(Stack.front());
}

std::string GiNaCexToString(const ex &x) {
  std::ostringstream Str;
  Str << x;
  return Str.str();
}

};

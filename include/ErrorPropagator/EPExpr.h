
#ifndef ERRORPROPAGATOR_EPEXPR_H
#define ERRORPROPAGATOR_EPEXPR_H

#include "llvm/Support/Casting.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <memory>
#include <string>

namespace ErrorProp {

using namespace llvm;

class LipschitzLoopStructure;

class EPExpr {
public:

  virtual std::string toString() const = 0;

  static std::unique_ptr<EPExpr>
  BuildEPExprFromLCSSA(const LipschitzLoopStructure &L, PHINode &PHI);

  static std::unique_ptr<EPExpr>
  BuildEPExprFromBodyValue(const LipschitzLoopStructure &L, Value &V);

  enum EPExprKind {
    K_EPBin,
    K_EPLeaf
  };

  EPExprKind getKind() const { return Kind; }

protected:
  EPExpr(EPExprKind K) : Kind(K) {}

private:
  const EPExprKind Kind;
};

class EPLeaf : public EPExpr {
public:

  EPLeaf(Value *E)
    : EPExpr(K_EPLeaf), External(E), Internal(nullptr) {
    assert(External != nullptr);
  }

  EPLeaf(Value *E, Value *I)
    : EPExpr(K_EPLeaf), External(E), Internal(I) {
    assert(External != nullptr && Internal != nullptr);
  }

  bool isLoopVariant() const { return Internal != nullptr; }

  std::string toString() const override;

  static bool classof(const EPExpr *E) { return E->getKind() == K_EPLeaf; }

protected:
  Value *External;
  Value *Internal;
};

class EPBin : public EPExpr {
public:

  EPBin(unsigned OpCode,
	std::unique_ptr<EPExpr> L,
	std::unique_ptr<EPExpr> R)
    : EPExpr(K_EPBin), Operation(OpCode),
      Left(std::move(L)), Right(std::move(R)) {
    assert(Left != nullptr && Right != nullptr);
  }

  unsigned getOperation() const { return Operation; }

  std::string toString() const override;

  static std::unique_ptr<EPExpr>
  BuildEPBin(const LipschitzLoopStructure &L, BinaryOperator &BO);

  static bool classof(const EPExpr *E) { return E->getKind() == K_EPBin; }

protected:
  const unsigned Operation;
  std::unique_ptr<EPExpr> Left;
  std::unique_ptr<EPExpr> Right;
};

} // end namespace ErrorProp

#endif

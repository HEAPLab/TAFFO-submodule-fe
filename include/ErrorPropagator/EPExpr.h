
#ifndef ERRORPROPAGATOR_EPEXPR_H
#define ERRORPROPAGATOR_EPEXPR_H

#include "llvm/Support/Casting.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include <memory>
#include <string>
#include <ginac/ginac.h>

#include "EPUtils/FixedPoint.h"

namespace ErrorProp {

using namespace llvm;

class LipschitzLoopStructure;

class EPExpr {
public:

  virtual std::string toString() const = 0;

  virtual GiNaC::ex
  toSymbolicEx(const MapVector<Value *, GiNaC::symbol> &Symbols) const = 0;

  static Value *
  BuildEPExprFromLCSSA(const LipschitzLoopStructure &L, PHINode &PHI,
		       DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs);

  static std::unique_ptr<EPExpr>
  BuildEPExprFromBodyValue(const LipschitzLoopStructure &L, Value &V,
			   DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
			   Instruction *Parent = nullptr);

  enum EPExprKind {
    K_EPBin,
    K_EPLeaf,
    K_EPConst
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

  bool isConstant() const {
    return !isLoopVariant() && isa<ConstantInt>(External);
  }

  Value *getExternal() const { return External; }

  Value *getInternal() const { return Internal; }

  GiNaC::ex
  toSymbolicEx(const MapVector<Value *, GiNaC::symbol> &Symbols) const override;

  std::string toString() const override;

  static std::unique_ptr<EPExpr>
  BuildExternalEPLeaf(Value *V,
		      DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs);

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

  EPExpr *getLeft() const { return Left.get(); }

  EPExpr *getRight() const { return Right.get(); }

  std::string toString() const override;

  GiNaC::ex
  toSymbolicEx(const MapVector<Value *, GiNaC::symbol> &Symbols) const override;

  static std::unique_ptr<EPExpr>
  BuildEPBin(const LipschitzLoopStructure &L, BinaryOperator &BO,
	     DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs);

  static bool classof(const EPExpr *E) { return E->getKind() == K_EPBin; }

protected:
  const unsigned Operation;
  std::unique_ptr<EPExpr> Left;
  std::unique_ptr<EPExpr> Right;
};

class EPConst : public EPExpr {
public:
  EPConst() : EPExpr(K_EPConst), Val(0) {}
  EPConst(inter_t V) : EPExpr(K_EPConst), Val(V) {}

  inter_t getValue() const { return Val; }

  std::string toString() const override;

  GiNaC::ex
  toSymbolicEx(const MapVector<Value *, GiNaC::symbol> &Symbols) const override;

  static std::unique_ptr<EPExpr>
  BuildEPConst(const ConstantInt &CI, const Instruction &I);

  static bool classof(const EPExpr *E) { return E->getKind() == K_EPConst; }

protected:
  inter_t Val;
};

} // end namespace ErrorProp

#endif

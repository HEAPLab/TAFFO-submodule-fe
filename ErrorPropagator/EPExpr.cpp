#include "ErrorPropagator/EPExpr.h"
#include "ErrorPropagator/Lipschitz.h"

namespace ErrorProp {

Value *
EPExpr::BuildEPExprFromLCSSA(const LipschitzLoopStructure &L, PHINode &PHI,
			     DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) {
  Value *BV = PHI.getIncomingValueForBlock(L.Body);

  Exprs.insert(std::make_pair(BV, BuildEPExprFromBodyValue(L, *BV, Exprs)));
  return BV;
}

std::unique_ptr<EPExpr>
EPExpr::BuildEPExprFromBodyValue(const LipschitzLoopStructure &L, Value &V,
				 DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs,
				 Instruction *Parent) {
  if (!V.getType()->isIntegerTy())
      return nullptr;

  if (isa<Instruction>(V)) {
    Instruction &I = cast<Instruction>(V);

    // If this instruction is outside of the loop,
    // we do not build its expression, but we add it as a single variable.
    if (I.getParent() != L.Body
	&& I.getParent() != L.Cond)
      return EPLeaf::BuildExternalEPLeaf(&I, Exprs);

    if (I.isBinaryOp()) {
      return EPBin::BuildEPBin(L, cast<BinaryOperator>(I), Exprs);
    }

    if (I.isLogicalShift()
	|| I.isArithmeticShift()
	|| I.getOpcode() == Instruction::Trunc
	|| I.getOpcode() == Instruction::SExt
	|| I.getOpcode() == Instruction::ZExt) {
      Value *Op = I.getOperand(0U);
      if (Op != nullptr)
	return BuildEPExprFromBodyValue(L, *Op, Exprs);
    }

    if (isa<PHINode>(I)) {
      // If it is not in the Cond BB, it means there are ifs/strange things in the loop.
      if (I.getParent() != L.Cond)
	return nullptr;

      PHINode &PHI = cast<PHINode>(I);
      Value *External = PHI.getIncomingValueForBlock(L.Predecessor);
      Value *BodyVar = PHI.getIncomingValueForBlock(L.Body);

      if (External != nullptr && BodyVar != nullptr)
	return std::unique_ptr<EPLeaf>(new EPLeaf(External, BodyVar));

      if (External != nullptr)
	return EPLeaf::BuildExternalEPLeaf(External, Exprs);

      return nullptr;
    }

    // No other instruction types are supported.
    return nullptr;
  }

  if (isa<ConstantInt>(V) && Parent != nullptr)
    return EPConst::BuildEPConst(cast<ConstantInt>(V), *Parent);

  if (isa<Argument>(V))
    return EPLeaf::BuildExternalEPLeaf(&V, Exprs);

  return nullptr;
}

std::unique_ptr<EPExpr>
EPLeaf::BuildExternalEPLeaf(Value *V,
			    DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) {
  Exprs.insert(std::make_pair(V, std::unique_ptr<EPLeaf>(new EPLeaf(V))));
  return std::unique_ptr<EPLeaf>(new EPLeaf(V));
}

std::unique_ptr<EPExpr>
EPBin::BuildEPBin(const LipschitzLoopStructure &L, BinaryOperator &BO,
		  DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) {
  // Check if this is a supported binary operator.
  unsigned OpCode = BO.getOpcode();
  if (!(OpCode == Instruction::Add
	|| OpCode == Instruction::Sub
	|| OpCode == Instruction::Mul
	|| OpCode == Instruction::SDiv
	|| OpCode == Instruction::UDiv))
    return nullptr;

  Value *LeftOp = BO.getOperand(0U);
  Value *RightOp = BO.getOperand(1U);
  if (LeftOp == nullptr || RightOp == nullptr)
    return nullptr;

  std::unique_ptr<EPExpr> Left =
    EPExpr::BuildEPExprFromBodyValue(L, *LeftOp, Exprs, &BO);
  std::unique_ptr<EPExpr> Right =
    EPExpr::BuildEPExprFromBodyValue(L, *RightOp, Exprs, &BO);
  if (Left == nullptr || Right == nullptr)
    return nullptr;

  return std::unique_ptr<EPBin>(new EPBin(OpCode, std::move(Left), std::move(Right)));
}

std::unique_ptr<EPExpr>
EPConst::BuildEPConst(const ConstantInt &CI, const Instruction &I) {
  std::unique_ptr<FixedPointValue> FPRange =
    FixedPointValue::createFromMetadata(I);
  if (FPRange == nullptr)
    return nullptr;

  int SPrec = (FPRange->isSigned()) ?
    -FPRange->getPointPos() : FPRange->getPointPos();
  std::unique_ptr<FixedPointValue> VFPRange =
    FixedPointValue::createFromConstantInt(SPrec, nullptr, &CI, &CI);
  if (VFPRange == nullptr)
    return nullptr;

  FPInterval VRange = VFPRange->getInterval();

  return std::unique_ptr<EPExpr>(new EPConst(VRange.Max));
}

std::string
EPLeaf::toString() const {
  if (this->isLoopVariant())
    return Internal->getName();
  else
    return External->getName();
}

std::string
EPBin::toString() const {
  std::string Op;
  switch(Operation) {
    case Instruction::Add:
      Op = " + ";
      break;
    case Instruction::Sub:
      Op = " - ";
      break;
    case Instruction::Mul:
      Op = " * ";
      break;
    case Instruction::SDiv:
    case Instruction::UDiv:
      Op = " / ";
      break;
    default:
      Op = " ? ";
  }
  return "(" + Left->toString() + Op + Right->toString() + ")";
}

std::string EPConst::toString() const {
  return std::to_string(Val);
}

GiNaC::ex
EPLeaf::toSymbolicEx(const DenseMap<Value *, GiNaC::symbol> &Symbols) const {
  if (this->isLoopVariant()) {
    auto VSym = Symbols.find(Internal);
    if (VSym == Symbols.end())
      return GiNaC::ex();

    return VSym->second;
  }
  else {
    auto VSym = Symbols.find(External);
    if (VSym == Symbols.end())
      return GiNaC::ex();

    return VSym->second;
  }
}

GiNaC::ex
EPBin::toSymbolicEx(const DenseMap<Value *, GiNaC::symbol> &Symbols) const {
  GiNaC::ex LeftEx = this->Left->toSymbolicEx(Symbols);
  GiNaC::ex RightEx = this->Right->toSymbolicEx(Symbols);

  switch (Operation) {
    case Instruction::Add:
      return LeftEx + RightEx;
    case Instruction::Sub:
      return LeftEx - RightEx;
    case Instruction::Mul:
      return LeftEx * RightEx;
    case Instruction::SDiv:
    case Instruction::UDiv:
      return LeftEx / RightEx;
  }
  llvm_unreachable("Invalid operation.");
}

GiNaC::ex
EPConst::toSymbolicEx(const DenseMap<Value *, GiNaC::symbol> &Symbols) const {
  return GiNaC::ex(static_cast<double>(Val));
}

} // end namespace ErrorProp

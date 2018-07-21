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
				 DenseMap<Value *, std::unique_ptr<EPExpr> > &Exprs) {
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

  if (isa<Constant>(V))
    return std::unique_ptr<EPLeaf>(new EPLeaf(&V));

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
    EPExpr::BuildEPExprFromBodyValue(L, *LeftOp, Exprs);
  std::unique_ptr<EPExpr> Right =
    EPExpr::BuildEPExprFromBodyValue(L, *RightOp, Exprs);
  if (Left == nullptr || Right == nullptr)
    return nullptr;

  return std::unique_ptr<EPBin>(new EPBin(OpCode, std::move(Left), std::move(Right)));
}

std::string
valueToString(Value *V) {
  assert(V != nullptr);

  ConstantInt *CI = dyn_cast<ConstantInt>(V);
  if (CI != nullptr)
    return std::to_string(CI->getSExtValue());

  return V->getName();
}

std::string
EPLeaf::toString() const {
  if (this->isLoopVariant())
    return valueToString(Internal);
  else
    return valueToString(External);
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

} // end namespace ErrorProp

#include "ErrorPropagator/EPExpr.h"
#include "ErrorPropagator/Lipschitz.h"

namespace ErrorProp {

std::unique_ptr<EPExpr>
EPExpr::BuildEPExprFromLCSSA(const LipschitzLoopStructure &L, PHINode &PHI) {
  Value *BV = PHI.getIncomingValueForBlock(L.Cond);

  if (isa<PHINode>(BV)) {
  // The incoming value for PHI is another PHINode from the Cond block.
  PHINode *CondPHI = dyn_cast<PHINode>(BV);
  if (CondPHI == nullptr)
    return nullptr;

  // Now we take the incoming value from the loop body.
  BV = CondPHI->getIncomingValueForBlock(L.Body);
  if (BV == nullptr)
    return nullptr;
  }

  return BuildEPExprFromBodyValue(L, *BV);
}

std::unique_ptr<EPExpr>
EPExpr::BuildEPExprFromBodyValue(const LipschitzLoopStructure &L, Value &V) {
  if (!V.getType()->isIntegerTy())
      return nullptr;

  if (isa<Instruction>(V)) {
    Instruction &I = cast<Instruction>(V);

    // If this instruction is outside of the loop,
    // we do not build its expression.
    if (I.getParent() != L.Body
	&& I.getParent() != L.Cond)
      return std::unique_ptr<EPLeaf>(new EPLeaf(&I));

    if (I.isBinaryOp()) {
      return EPBin::BuildEPBin(L, cast<BinaryOperator>(I));
    }

    if (I.isLogicalShift()
	|| I.isArithmeticShift()
	|| I.getOpcode() == Instruction::Trunc
	|| I.getOpcode() == Instruction::SExt
	|| I.getOpcode() == Instruction::ZExt) {
      Value *Op = I.getOperand(0U);
      if (Op != nullptr)
	return BuildEPExprFromBodyValue(L, *Op);
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
	return std::unique_ptr<EPLeaf>(new EPLeaf(External));

      return nullptr;
    }

    // No other instruction types are supported.
    return nullptr;
  }

  if (isa<Argument>(V) || isa<Constant>(V))
    return std::unique_ptr<EPLeaf>(new EPLeaf(&V));

  return nullptr;
}

std::unique_ptr<EPExpr>
EPBin::BuildEPBin(const LipschitzLoopStructure &L, BinaryOperator &BO) {
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

  std::unique_ptr<EPExpr> Left = EPExpr::BuildEPExprFromBodyValue(L, *LeftOp);
  std::unique_ptr<EPExpr> Right = EPExpr::BuildEPExprFromBodyValue(L, *RightOp);
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

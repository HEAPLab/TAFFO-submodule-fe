#include "ErrorPropagator/EPUtils/InputInfo.h"

#include <cmath>
#include "llvm/IR/Constants.h"

namespace ErrorProp {

using namespace llvm;

std::unique_ptr<TType> TType::createFromMetadata(MDNode *MDN) {
  if (FPType::isFPTypeMetadata(MDN))
    return FPType::createFromMetadata(MDN);

  llvm_unreachable("Unsupported data type.");
}

bool FPType::isFPTypeMetadata(MDNode *MDN) {
  if (MDN->getNumOperands() < 1)
    return false;

  MDString *Flag = cast<MDString>(MDN->getOperand(0U).get());
  return Flag->getString().equals(FIXP_TYPE_FLAG);
}

std::unique_ptr<FPType> FPType::createFromMetadata(MDNode *MDN) {
  assert(isFPTypeMetadata(MDN) && "Must be of fixp type.");
  assert(MDN->getNumOperands() >= 3U && "Must have flag, width, PointPos.");

  int Width;
  Metadata *WMD = MDN->getOperand(1U).get();
  ConstantAsMetadata *WCMD = cast<ConstantAsMetadata>(WMD);
  ConstantInt *WCI = cast<ConstantInt>(WCMD->getValue());
  Width = WCI->getSExtValue();

  unsigned PointPos;
  Metadata *PMD = MDN->getOperand(2U).get();
  ConstantAsMetadata *PCMD = cast<ConstantAsMetadata>(PMD);
  ConstantInt *PCI = cast<ConstantInt>(PCMD->getValue());
  PointPos = PCI->getZExtValue();

  return std::unique_ptr<FPType>(new FPType(Width, PointPos));
}

MDNode *FPType::toMetadata(LLVMContext &C) const {
  Metadata *TypeFlag = MDString::get(C, FIXP_TYPE_FLAG);

  IntegerType *Int32Ty = Type::getInt32Ty(C);
  ConstantInt *WCI = ConstantInt::getSigned(Int32Ty, this->getSWidth());
  Metadata *WidthMD = ConstantAsMetadata::get(WCI);

  ConstantInt *PCI = ConstantInt::get(Int32Ty, this->getPointPos());
  ConstantAsMetadata *PointPosMD = ConstantAsMetadata::get(PCI);

  Metadata *MDs[] = {TypeFlag, WidthMD, PointPosMD};
  return MDNode::get(C, MDs);
}

double FPType::getRoundingError() const {
  return std::ldexp(1.0, -this->getPointPos());
}

Metadata *createDoubleMetadata(LLVMContext &C, double Value) {
  Type *DoubleTy = Type::getDoubleTy(C);
  Constant *ValC = ConstantFP::get(DoubleTy, Value);
  return ConstantAsMetadata::get(ValC);
}

MDNode *createDoubleMDNode(LLVMContext &C, double Value) {
  return MDNode::get(C, createDoubleMetadata(C, Value));
}

double retrieveDoubleMetadata(Metadata *DMD) {
  ConstantAsMetadata *DCMD = cast<ConstantAsMetadata>(DMD);
  ConstantFP *DCFP = cast<ConstantFP>(DCMD->getValue());
  return DCFP->getValueAPF().convertToDouble();
}

double retrieveDoubleMDNode(MDNode *MDN) {
  assert(MDN != nullptr);
  assert(MDN->getNumOperands() > 0 && "Must have at least one operand.");

  return retrieveDoubleMetadata(MDN->getOperand(0U).get());
}

std::unique_ptr<Range> Range::createFromMetadata(MDNode *MDN) {
  assert(MDN != nullptr);
  assert(MDN->getNumOperands() == 2U && "Must contain Min and Max.");

  double Min = retrieveDoubleMetadata(MDN->getOperand(0U).get());
  double Max = retrieveDoubleMetadata(MDN->getOperand(1U).get());
  return std::unique_ptr<Range>(new Range(Min, Max));
}

MDNode *Range::toMetadata(LLVMContext &C) const {
  Metadata *RangeMD[] = {createDoubleMetadata(C, this->Min),
			 createDoubleMetadata(C, this->Max)};
  return MDNode::get(C, RangeMD);
}

std::unique_ptr<double> CreateInitialErrorFromMetadata(MDNode *MDN) {
  return std::unique_ptr<double>(new double(retrieveDoubleMDNode(MDN)));
}

MDNode *InitialErrorToMetadata(LLVMContext &C, double Error) {
    return createDoubleMDNode(C, Error);
}

MDNode *InputInfo::toMetadata(LLVMContext &C) const {
  Metadata *Null = ConstantAsMetadata::get(ConstantInt::getFalse(C));
  Metadata *TypeMD = (IType) ? IType->toMetadata(C) : Null;
  Metadata *RangeMD = (IRange) ? IRange->toMetadata(C) : Null;
  Metadata *ErrorMD = (IError) ? InitialErrorToMetadata(C, *IError) : Null;

  Metadata *InputMDs[] = {TypeMD, RangeMD, ErrorMD};
  return MDNode::get(C, InputMDs);
}

std::unique_ptr<CmpErrorInfo> CmpErrorInfo::createFromMetadata(MDNode *MDN) {
  if (MDN == nullptr)
    return std::unique_ptr<CmpErrorInfo>(new CmpErrorInfo(0.0, false));

  double MaxTol = retrieveDoubleMDNode(MDN);
  return std::unique_ptr<CmpErrorInfo>(new CmpErrorInfo(MaxTol));
}

MDNode *CmpErrorInfo::toMetadata(LLVMContext &C) const {
  return createDoubleMDNode(C, MaxTolerance);
}

} // end namespace ErrorProp

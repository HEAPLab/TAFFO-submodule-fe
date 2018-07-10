//===-- Metadata.cpp - Metadata Utils for ErrorPropagator -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definitions of utility functions that handle metadata in Error Propagator.
///
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/ErrorPropagator/Metadata.h"

namespace ErrorProp {

using namespace llvm;

MDNode *createErrorMetadata(LLVMContext &Context,
			    const AffineForm<inter_t> &E) {
  Type *DoubleType = Type::getDoubleTy(Context);
  Constant *ErrConst =
    ConstantFP::get(DoubleType,
		    static_cast<double>(E.noiseTermsAbsSum()));
  ConstantAsMetadata *ErrMD = ConstantAsMetadata::get(ErrConst);
  return MDNode::get(Context, ErrMD);
}

void setErrorMetadata(Instruction &I, const AffineForm<inter_t> &E) {
  MDNode *ErrMDN = createErrorMetadata(I.getContext(), E);
  I.setMetadata(COMP_ERROR_METADATA, ErrMDN);
}

MDNode *getRangeErrorMetadata(LLVMContext &Context,
			      std::pair<
			      const FixedPointValue *,
			      const AffineForm<inter_t> *> &RE) {
  MDNode *RangeMD = RE.first->toMetadata(Context);
  MDNode *ErrorMD = createErrorMetadata(Context, *RE.second);
  Metadata *REMD[] = {RangeMD, ErrorMD};
  return MDNode::get(Context, REMD);
}

MDTuple *getArgsMetadata(LLVMContext &Context,
			 const ArrayRef<std::pair<
			 const FixedPointValue *,
			 const AffineForm<inter_t> *> > Data) {
  SmallVector<Metadata *, 2U> AllArgsMD;
  AllArgsMD.reserve(Data.size());

  for (auto RE : Data)
    AllArgsMD.push_back(getRangeErrorMetadata(Context, RE));

  return MDNode::get(Context, AllArgsMD);
}

void setFunctionArgsMetadata(Function &F,
			     const ArrayRef<std::pair<
			     const FixedPointValue *,
			     const AffineForm<inter_t> *> > Data) {
  assert(F.arg_size() <= Data.size()
	 && "Range and Error data must be supplied for each argument.");

  F.setMetadata(FUNCTION_ARGS_METADATA,
		getArgsMetadata(F.getContext(), Data));
}

AffineForm<inter_t>
getArgErrorFromMetadata(const MDNode &ErrMD) {
  ConstantAsMetadata *ErrCMD = cast<ConstantAsMetadata>(ErrMD.getOperand(0).get());

  ConstantFP *ErrCFP = cast<ConstantFP>(ErrCMD->getValue());
  double Err = ErrCFP->getValueAPF().convertToDouble();

  return AffineForm<inter_t>(0, Err);
}

std::pair<FPInterval, AffineForm<inter_t> >
getArgRangeErrorFromMetadata(const MDNode &ArgMD) {
  assert(ArgMD.getNumOperands() >= 2
	 && "Range and Error must be supplied for each argument.");

  std::unique_ptr<FixedPointValue> FPVRange =
    FixedPointValue::createFromMDNode(nullptr,
				      *cast<MDNode>(ArgMD.getOperand(0U).get()));
  assert(FPVRange != nullptr && "Malformed argument range.");

  FPInterval ArgRange = FPVRange->getInterval();

  AffineForm<inter_t> ArgErr =
    getArgErrorFromMetadata(*cast<MDNode>(ArgMD.getOperand(1U).get()));

  return std::make_pair(ArgRange, ArgErr);
}

SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const Function &F) {
  assert(propagateFunction(F) && "Must contain argument metadata.");

  MDNode *ArgsMD = F.getMetadata(FUNCTION_ARGS_METADATA);

  SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U> REs;
  for (auto ArgMDOp = ArgsMD->op_begin(), ArgMDOpEnd = ArgsMD->op_end();
       ArgMDOp != ArgMDOpEnd; ++ArgMDOp) {
    MDNode *ArgMDNode = cast<MDNode>(ArgMDOp->get());
    REs.push_back(getArgRangeErrorFromMetadata(*ArgMDNode));
  }
  return std::move(REs);
}

bool propagateFunction(const Function &F) {
  return F.getMetadata(FUNCTION_ARGS_METADATA) != nullptr;
}

void setCmpErrorMetadata(Instruction &I, const CmpErrorInfo &CmpInfo) {
  if (!CmpInfo.MayBeWrong)
    return;

  Type *DoubleType = Type::getDoubleTy(I.getContext());
  Constant *MaxTolConst =
    ConstantFP::get(DoubleType,
		    static_cast<double>(CmpInfo.MaxTolerance));
  ConstantAsMetadata *ErrMD = ConstantAsMetadata::get(MaxTolConst);
  MDNode *MaxTolNode = MDNode::get(I.getContext(), ErrMD);

  I.setMetadata(WRONG_CMP_METADATA, MaxTolNode);
}

void setGlobalVariableMetadata(GlobalObject &V,
			       const FixedPointValue *Range,
			       const AffineForm<inter_t> *Error) {
  assert(Range != nullptr && "Null Range pointer.");
  assert(Error != nullptr && "Null Error pointer.");

  auto RE = std::make_pair(Range, Error);
  MDNode *GMD = getRangeErrorMetadata(V.getContext(), RE);
  assert(GMD != nullptr && "Null metadata was returned.");

  V.setMetadata(GLOBAL_VAR_METADATA, GMD);
}

bool
hasGlobalVariableMetadata(const GlobalObject &V) {
  return V.getMetadata(GLOBAL_VAR_METADATA) != nullptr;
}

std::pair<FPInterval, AffineForm<inter_t> >
retrieveGlobalVariableRangeError(const GlobalObject &V) {
  MDNode *MD = V.getMetadata(GLOBAL_VAR_METADATA);
  assert(MD != nullptr && "No global variable metadata.");
  return getArgRangeErrorFromMetadata(*MD);
}

}

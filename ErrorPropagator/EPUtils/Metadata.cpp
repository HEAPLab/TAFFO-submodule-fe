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

#include "ErrorPropagator/EPUtils/Metadata.h"

namespace ErrorProp {

using namespace llvm;

Metadata *createDoubleMetadata(LLVMContext &C, const inter_t &Value) {
  Type *DoubleTy = Type::getDoubleTy(C);
  Constant *ValC = ConstantFP::get(DoubleTy,
				   static_cast<double>(Value));
  return ConstantAsMetadata::get(ValC);
}

double retrieveDoubleMetadata(Metadata *DMD) {
  ConstantAsMetadata *DCMD = cast<ConstantAsMetadata>(DMD);
  ConstantFP *DCFP = cast<ConstantFP>(DCMD->getValue());
  return DCFP->getValueAPF().convertToDouble();
}

MDNode *createRangeMetadata(LLVMContext &C, const Interval<inter_t> &Range) {
  Metadata *RangeMD[] = {createDoubleMetadata(C, Range.Min),
			 createDoubleMetadata(C, Range.Max)};
  return MDNode::get(C, RangeMD);
}

MDNode *createDoubleMDNode(LLVMContext &C, const inter_t &Value) {
  return MDNode::get(C, createDoubleMetadata(C, Value));
}

double retrieveDoubleMDNode(MDNode *MDN) {
  assert(MDN != nullptr);
  assert(MDN->getNumOperands() > 0 && "Must have at least one operand.");

  return retrieveDoubleMetadata(MDN->getOperand(0U).get());
}

Metadata *createNullFieldMetadata(LLVMContext &C) {
  return ConstantAsMetadata::get(ConstantInt::getFalse(C));
}

MDNode *createInputInfoMetadata(LLVMContext &C, const InputInfo &IInfo) {
  Metadata *Null = createNullFieldMetadata(C);
  Metadata *TypeMD = (IInfo.Ty) ? IInfo.Ty->toMetadata(C) : Null;
  Metadata *RangeMD = (IInfo.Range) ? createRangeMetadata(C, *IInfo.Range) : Null;
  Metadata *ErrorMD = (IInfo.Error) ? createDoubleMDNode(C, *IInfo.Error) : Null;

  Metadata *InputMDs[] = {TypeMD, RangeMD, ErrorMD};
  return MDNode::get(C, InputMDs);
}

void setInputInfoMetadata(Instruction &I, const InputInfo &IInfo) {
  I.setMetadata(INPUT_INFO_METADATA,
		createInputInfoMetadata(I.getContext(), IInfo));
}

Optional<FPInterval> retrieveRangeFromMetadata(const MDNode *MDN) {
  if (MDN == nullptr)
    return NoneType();
  assert(MDN->getNumOperands() == 3U
	 && "Must contain type info, range, initial error.");

  FPType TypeInfo =
    FPType::createFromMetadata(cast<MDNode>(MDN->getOperand(0U).get()));

  MDNode *RangeMDN = cast<MDNode>(MDN->getOperand(1U).get());
  assert(RangeMDN->getNumOperands() == 2U && "Must have Min and Max.");
  Interval<inter_t> Range(retrieveDoubleMetadata(RangeMDN->getOperand(0U).get()),
			  retrieveDoubleMetadata(RangeMDN->getOperand(1U).get()));

  return FPInterval(TypeInfo, Range);
}

Optional<FPInterval> retrieveRangeFromMetadata(Instruction &I) {
  MDNode *MDN = I.getMetadata(RANGE_METADATA);
  return retrieveRangeFromMetadata(MDN);
}

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

MDTuple *getArgsMetadata(LLVMContext &Context,
			 const ArrayRef<InputInfo> AInfo) {
  SmallVector<Metadata *, 2U> AllArgsMD;
  AllArgsMD.reserve(AInfo.size());

  for (auto &IInfo : AInfo)
    AllArgsMD.push_back(createInputInfoMetadata(Context, IInfo));

  return MDNode::get(Context, AllArgsMD);
}

void setFunctionArgsMetadata(Function &F,
			     const ArrayRef<InputInfo> AInfo) {
  F.setMetadata(FUNCTION_ARGS_METADATA,
		getArgsMetadata(F.getContext(), AInfo));
}

std::pair<FPInterval, AffineForm<inter_t> >
getArgRangeErrorFromMetadata(const MDNode &ArgII) {
  assert(ArgII.getNumOperands() == 3U
	 && "Must contain type info, range, initial error.");

  Optional<FPInterval> FPR = retrieveRangeFromMetadata(&ArgII);
  assert(FPR.hasValue() && "Malformed argument range.");

  AffineForm<inter_t> ArgErr =
    AffineForm<inter_t>(0, retrieveDoubleMDNode(cast<MDNode>(ArgII.getOperand(2U).get())));

  return std::make_pair(FPR.getValue(), ArgErr);
}

SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const Function &F) {
  SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U> REs;

  MDNode *ArgsMD = F.getMetadata(FUNCTION_ARGS_METADATA);
  if (ArgsMD == nullptr)
    return std::move(REs);

  for (auto ArgMDOp = ArgsMD->op_begin(), ArgMDOpEnd = ArgsMD->op_end();
       ArgMDOp != ArgMDOpEnd; ++ArgMDOp) {
    MDNode *ArgMDNode = cast<MDNode>(ArgMDOp->get());
    REs.push_back(getArgRangeErrorFromMetadata(*ArgMDNode));
  }
  return std::move(REs);
}

// bool propagateFunction(const Function &F) {
//   return F.getMetadata(FUNCTION_ARGS_METADATA) != nullptr;
// }

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

void setGlobalVariableMetadata(GlobalObject &V, const InputInfo &IInfo) {
  V.setMetadata(GLOBAL_VAR_METADATA,
		createInputInfoMetadata(V.getContext(), IInfo));
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

void
setMaxRecursionCountMetadata(Function &F, unsigned MaxRecursionCount) {
  ConstantInt *CIRC = ConstantInt::get(Type::getInt32Ty(F.getContext()),
				       MaxRecursionCount,
				       false);
  ConstantAsMetadata *CMRC = ConstantAsMetadata::get(CIRC);
  MDNode *RCNode = MDNode::get(F.getContext(), CMRC);
  F.setMetadata(MAX_REC_METADATA, RCNode);
}

unsigned
retrieveMaxRecursionCount(const Function &F) {
  MDNode *RecC = F.getMetadata(MAX_REC_METADATA);
  if (RecC == nullptr)
    return 0U;

  assert(RecC->getNumOperands() > 0 && "Must contain the recursion count.");
  ConstantAsMetadata *CMRC = cast<ConstantAsMetadata>(RecC->getOperand(0U));
  ConstantInt *CIRC = cast<ConstantInt>(CMRC->getValue());
  return CIRC->getZExtValue();
}

void
setLoopUnrollCountMetadata(Loop &L, unsigned UnrollCount) {
  // Get Loop header terminating instruction
  BasicBlock *Header = L.getHeader();
  assert(Header && "Loop with no header.");

  TerminatorInst *HTI = Header->getTerminator();
  assert(HTI && "Block with no terminator");

  // Prepare MD Node
  ConstantInt *CIUC = ConstantInt::get(Type::getInt32Ty(HTI->getContext()),
				       UnrollCount,
				       false);
  ConstantAsMetadata *CMUC = ConstantAsMetadata::get(CIUC);
  MDNode *UCNode = MDNode::get(HTI->getContext(), CMUC);

  HTI->setMetadata(UNROLL_COUNT_METADATA, UCNode);
}

Optional<unsigned>
retrieveLoopUnrollCount(const Loop &L) {
  // Get Loop header terminating instruction
  BasicBlock *Header = L.getHeader();
  assert(Header && "Loop with no header.");

  TerminatorInst *HTI = Header->getTerminator();
  assert(HTI && "Block with no terminator");

  MDNode *UCNode = HTI->getMetadata(UNROLL_COUNT_METADATA);
  if (UCNode == nullptr)
    return NoneType();

  assert(UCNode->getNumOperands() > 0 && "Must contain the unroll count.");
  ConstantAsMetadata *CMUC = cast<ConstantAsMetadata>(UCNode->getOperand(0U));
  ConstantInt *CIUC = cast<ConstantInt>(CMUC->getValue());
  return CIUC->getZExtValue();
}

}

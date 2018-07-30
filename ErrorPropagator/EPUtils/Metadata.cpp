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

Optional<FPInterval> retrieveRangeFromMetadata(MDNode *MDN) {
  if (MDN == nullptr)
    return NoneType();
  assert(MDN->getNumOperands() == 4U
	 && "Must contain width, point pos. and bounds.");

  int Width;
  Metadata *WMD = MDN->getOperand(0U).get();
  ConstantAsMetadata *WCMD = cast<ConstantAsMetadata>(WMD);
  ConstantInt *WCI = cast<ConstantInt>(WCMD->getValue());
  Width = WCI->getSExtValue();

  unsigned PointPos;
  Metadata *PMD = MDN->getOperand(1U).get();
  ConstantAsMetadata *PCMD = cast<ConstantAsMetadata>(WMD);
  ConstantInt *PCI = cast<ConstantInt>(PCMD->getValue());
  PointPos = PCI->getZExtValue();

  inter_t Min;
  Metadata *MiMD = MDN->getOperand(2U).get();
  ConstantAsMetadata *MiCMD = cast<ConstantAsMetadata>(MiMD);
  ConstantFP *MiCFP = cast<ConstantFP>(MiCMD->getValue());
  Min = MiCFP->getValueAPF().convertToDouble();

  inter_t Max;
  Metadata *MaMD = MDN->getOperand(3U).get();
  ConstantAsMetadata *MaCMD = cast<ConstantAsMetadata>(MaMD);
  ConstantFP *MaCFP = cast<ConstantFP>(MaCMD->getValue());
  Max = MaCFP->getValueAPF().convertToDouble();

  return FPInterval(FPType(Width, PointPos),
		    Interval<inter_t>(Min, Max));
}

Optional<FPInterval> retrieveRangeFromMetadata(Instruction &I) {
  MDNode *MDN = I.getMetadata(RANGE_METADATA);
  return retrieveRangeFromMetadata(MDN);
}

MDNode *createRangeMetadata(LLVMContext &C, const FPInterval &FPI) {
  IntegerType *Int32Ty = Type::getInt32Ty(C);

  // Width
  ConstantInt *WCI = ConstantInt::getSigned(Int32Ty, FPI.getFPType().getSWidth());
  ConstantAsMetadata *WCMD = ConstantAsMetadata::get(WCI);

  // PointPos
  ConstantInt *PCI = ConstantInt::get(Int32Ty, FPI.getPointPos());
  ConstantAsMetadata *PCMD = ConstantAsMetadata::get(PCI);

  Type *DoubleTy = Type::getDoubleTy(C);
  // Min
  Constant *MiCFP = ConstantFP::get(DoubleTy, static_cast<double>(FPI.Min));
  ConstantAsMetadata *MiCMD = ConstantAsMetadata::get(MiCFP);

  // Max
  Constant *MaCFP = ConstantFP::get(DoubleTy, static_cast<double>(FPI.Max));
  ConstantAsMetadata *MaCMD = ConstantAsMetadata::get(MaCFP);

  Metadata *MDS[] = {WCMD, PCMD, MiCMD, MaCMD};
  return MDNode::get(C, MDS);
}

void setRangeMetadata(Instruction &I, const FPInterval &FPI) {
  I.setMetadata(RANGE_METADATA,
		createRangeMetadata(I.getContext(), FPI));
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

MDNode *getRangeErrorMetadata(LLVMContext &Context,
			      std::pair<
			      const FPInterval *,
			      const AffineForm<inter_t> *> &RE) {
  MDNode *RangeMD = createRangeMetadata(Context, *RE.first);
  MDNode *ErrorMD = createErrorMetadata(Context, *RE.second);
  Metadata *REMD[] = {RangeMD, ErrorMD};
  return MDNode::get(Context, REMD);
}

MDTuple *getArgsMetadata(LLVMContext &Context,
			 const ArrayRef<std::pair<
			 const FPInterval *,
			 const AffineForm<inter_t> *> > Data) {
  SmallVector<Metadata *, 2U> AllArgsMD;
  AllArgsMD.reserve(Data.size());

  for (auto RE : Data)
    AllArgsMD.push_back(getRangeErrorMetadata(Context, RE));

  return MDNode::get(Context, AllArgsMD);
}

void setFunctionArgsMetadata(Function &F,
			     const ArrayRef<std::pair<
			     const FPInterval *,
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

  Optional<FPInterval> FPR =
    retrieveRangeFromMetadata(cast<MDNode>(ArgMD.getOperand(0U).get()));
  assert(FPR.hasValue() && "Malformed argument range.");

  AffineForm<inter_t> ArgErr =
    getArgErrorFromMetadata(*cast<MDNode>(ArgMD.getOperand(1U).get()));

  return std::make_pair(FPR.getValue(), ArgErr);
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
			       const FPInterval *Range,
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

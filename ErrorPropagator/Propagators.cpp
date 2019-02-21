//===-- Propagators.cpp - Propagators for LLVM Instructions -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Definitions of functions that propagate fixed point computation errors
/// for each LLVM Instruction.
///
//===----------------------------------------------------------------------===//

#include "Propagators.h"

#include "llvm/Support/Debug.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "AffineForms.h"
#include "Metadata.h"
#include "MemSSAUtils.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

namespace {

using namespace llvm;
using namespace mdutils;

AffineForm<inter_t>
propagateAdd(const FPInterval &R1, const AffineForm<inter_t> &E1,
	     const FPInterval&, const AffineForm<inter_t> &E2) {
  // The absolute errors must be summed one by one
  return E1 + E2;
}

AffineForm<inter_t>
propagateSub(const FPInterval &R1, const AffineForm<inter_t> &E1,
	     const FPInterval&, const AffineForm<inter_t> &E2) {
  // The absolute errors must be subtracted one by one
  return E1 - E2;
}

AffineForm<inter_t>
propagateMul(const FPInterval &R1, const AffineForm<inter_t> &E1,
	     const FPInterval &R2, const AffineForm<inter_t> &E2) {
  // With x = y * z, the new error for x is computed as
  // errx = y*errz + x*erry + erry*errz
  return AffineForm<inter_t>(R1) * E2
    + AffineForm<inter_t>(R2) * E1
    + E1 * E2;
}

AffineForm<inter_t>
propagateDiv(const FPInterval &R1, const AffineForm<inter_t> &E1,
	     const FPInterval &R2, const AffineForm<inter_t> &E2,
	     bool AddTrunc = true) {
  // Compute y / z as y * 1/z.

  // Compute the range of 1/z.
  FPInterval InvR2(R2);
  InvR2.Min = 1.0 / R2.Max;
  InvR2.Max = 1.0 / R2.Min;

  // Compute errors on 1/z.
  AffineForm<inter_t> E1OverZ =
    LinearErrorApproximationDecr([](inter_t x){ return static_cast<inter_t>(-1) / (x * x); },
				 R2, E2);

  // The error for y / z will be
  // x * err1/z + 1/z * errx + errx * err1/z
  // plus the rounding error due to truncation.
  AffineForm<inter_t> Res = AffineForm<inter_t>(R1) * E1OverZ
    + AffineForm<inter_t>(InvR2) * E1
    + E1 * E1OverZ;

  if (AddTrunc)
    return Res + AffineForm<inter_t>(0, R1.getRoundingError());
  else
    return std::move(Res);
}

AffineForm<inter_t>
propagateShl(const AffineForm<inter_t> &E1) {
  // When shifting left there is no loss of precision (if no overflow occurs).
  return E1;
}

/// Propagate Right Shift Instruction.
/// \param E1 Errors associated to the operand to be shifted.
/// \param ResR Range of the result, only used to obtain target point position.
AffineForm<inter_t>
propagateShr(const AffineForm<inter_t> &E1, const FPInterval &ResR) {
  return E1 + AffineForm<inter_t>(0, ResR.getRoundingError());
}

} // end of anonymous namespace

bool InstructionPropagator::propagateBinaryOp(Instruction &I) {
  BinaryOperator &BI = cast<BinaryOperator>(I);

  DEBUG(dbgs() << "Computing error for " << BI.getOpcodeName()
	<< " instruction " << I.getName() << "... ");

  // if (RMap.getRangeError(&I) == nullptr) {
  //   DEBUG(dbgs() << "ignored (no range data).\n");
  //   return false;
  // }

  bool DoublePP = BI.getOpcode() == Instruction::UDiv
    || BI.getOpcode() == Instruction::SDiv;
  auto *O1 = getOperandRangeError(BI, 0U, DoublePP);
  auto *O2 = getOperandRangeError(BI, 1U);
  if (O1 == nullptr || !O1->second.hasValue()
      || O2 == nullptr || !O2->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> ERes;
  switch(BI.getOpcode()) {
    case Instruction::FAdd:
      // Fall-through.
    case Instruction::Add:
      ERes = propagateAdd(O1->first, *O1->second,
			  O2->first, *O2->second);
      break;
    case Instruction::FSub:
      // Fall-through.
    case Instruction::Sub:
      ERes = propagateSub(O1->first, *O1->second,
			  O2->first, *O2->second);
      break;
    case Instruction::FMul:
      // Fall-through.
    case Instruction::Mul:
      ERes = propagateMul(O1->first, *O1->second,
			  O2->first, *O2->second);
      break;
    case Instruction::FDiv:
      ERes = propagateDiv(O1->first, *O1->second,
			  O2->first, *O2->second, false);
      break;
    case Instruction::UDiv:
      // Fall-through.
    case Instruction::SDiv:
      ERes = propagateDiv(O1->first, *O1->second,
			  O2->first, *O2->second);
      break;
    case Instruction::Shl:
      ERes = propagateShl(*O1->second);
      break;
    case Instruction::LShr:
      // Fall-through.
    case Instruction::AShr: {
      const FPInterval *ResR = RMap.getRange(&I);
      if (ResR == nullptr) {
	DEBUG(dbgs() << "no data.");
	return false;
      }
      ERes = propagateShr(*O1->second, *ResR);
      break;
    }
    default:
      DEBUG(dbgs() << "not supported.\n");
      return false;
  }

  // Add error to RMap.
  RMap.setError(&BI, ERes);

  DEBUG(dbgs() << static_cast<double>(ERes.noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagateStore(Instruction &I) {
  assert(I.getOpcode() == Instruction::Store && "Must be Store.");
  StoreInst &SI = cast<StoreInst>(I);

  DEBUG(dbgs() << "Propagating error for Store instruction " << I.getName() << "... ");

  Value *IDest = SI.getPointerOperand();
  assert(IDest != nullptr && "Store with null Pointer Operand.\n");
  auto *PointerRE = RMap.getRangeError(IDest);

  auto *SrcRE = getOperandRangeError(I, 0U);
  if (SrcRE == nullptr || !SrcRE->second.hasValue()) {
    DEBUG(dbgs() << "(no data, looking up pointer) ");
    SrcRE = PointerRE;
    if (SrcRE == nullptr || !SrcRE->second.hasValue()) {
      DEBUG(dbgs() << " ignored (no data).\n");
      return false;
    }
  }
  assert(SrcRE != nullptr);

  // Associate the source error to this store instruction,
  RMap.setRangeError(&SI, *SrcRE);
  // and to the pointer, if greater, and if it is a function Argument.
  updateArgumentRE(IDest, SrcRE);

  DEBUG(dbgs() << static_cast<double>(SrcRE->second->noiseTermsAbsSum()) << ".\n");

  // Update struct errors.
  RMap.setStructRangeError(SI.getPointerOperand(), *SrcRE);

  return true;
}


bool InstructionPropagator::propagateLoad(Instruction &I) {
  assert(I.getOpcode() == Instruction::Load && "Must be Load.");
  LoadInst &LI = cast<LoadInst>(I);

  if (isa<PointerType>(LI.getType())) {
    DEBUG(dbgs() << "Pointer load " << I.getName() << " ignored.\n");
    return false;
  }

  DEBUG(dbgs() << "Propagating error for Load instruction " << I.getName() << "... ");

  // Look for range and error in the defining instructions with MemorySSA
  MemSSAUtils MemUtils(RMap, MemSSA);
  MemUtils.findMemSSAError(&I, MemSSA.getMemoryAccess(&I));
  MemSSAUtils::REVector &REs = MemUtils.getRangeErrors();

  // Kludje for when AliasAnalysis fails (i.e. almost always).
  // findLOEError(RMap, &I, REs);

  // If this is a load of a struct element, lookup in the struct errors.
  if (const RangeErrorMap::RangeError *StructRE
      = RMap.getStructRangeError(LI.getPointerOperand())) {
    REs.push_back(StructRE);
    DEBUG(dbgs() << "(StructError: "
	  << static_cast<double>(StructRE->second.getValue().noiseTermsAbsSum()) << ") ");
  }

  std::sort(REs.begin(), REs.end());
  auto NewEnd = std::unique(REs.begin(), REs.end());
  REs.resize(NewEnd - REs.begin());

  if (REs.size() == 1U && REs.front() != nullptr) {
    // If we found only one defining instruction, we just use its data.
    const RangeErrorMap::RangeError *RE = REs.front();
    RMap.setRangeError(&I, *RE);
    DEBUG(if (RE->second.hasValue())
	    dbgs() << "(one value) " << static_cast<double>(RE->second->noiseTermsAbsSum()) << ".\n";
	  else
	    dbgs() << "no data.\n");
    return true;
  }

  // Otherwise, we take the maximum error.
  inter_t MaxAbsErr = -1.0;
  for (const RangeErrorMap::RangeError *RE : REs)
    if (RE != nullptr && RE->second.hasValue()) {
      MaxAbsErr = std::max(MaxAbsErr, RE->second->noiseTermsAbsSum());
    }

  // We also take the range from metadata attached to LI (if any).
  const FPInterval *SrcR = RMap.getRange(&I);
  if (SrcR == nullptr) {
    DEBUG(dbgs() << "ignored (no data).\n");
    return false;
  }

  if (MaxAbsErr >= 0) {
    AffineForm<inter_t> Error(0, MaxAbsErr);
    RMap.setRangeError(&I, std::make_pair(*SrcR, Error));

    DEBUG(dbgs() << static_cast<double>(Error.noiseTermsAbsSum()) << ".\n");
    return true;
  }

  // If we have no other error info, we take the rounding error.
  AffineForm<inter_t> Error(0, SrcR->getRoundingError());
  RMap.setRangeError(&I, std::make_pair(*SrcR, Error));
  DEBUG(dbgs() << "(no data, falling back to rounding error) "
	<< static_cast<double>(Error.noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagateExt(Instruction &I) {
  assert((I.getOpcode() == Instruction::SExt
	  || I.getOpcode() == Instruction::ZExt
	  || I.getOpcode() == Instruction::FPExt)
	 && "Must be SExt, ZExt or FExt.");

  DEBUG(dbgs() << "Propagating error for Extend instruction " << I.getName() << "... ");

  // No further error is introduced with signed/unsigned extension.
  return unOpErrorPassThrough(I);
}

bool InstructionPropagator::propagateTrunc(Instruction &I) {
  assert((I.getOpcode() == Instruction::Trunc
	  || I.getOpcode() == Instruction::FPTrunc)
	 && "Must be Trunc.");

  DEBUG(dbgs() << "Propagating error for Trunc instruction " << I.getName() << "... ");

  // No further error is introduced with truncation if no overflow occurs
  // (in which case it is useless to propagate other errors).
  return unOpErrorPassThrough(I);
}

bool InstructionPropagator::propagateIToFP(Instruction &I) {
  assert((isa<SIToFPInst>(I) || isa<UIToFPInst>(I)) && "Must be IToFP.");

  DEBUG(dbgs() << "Propagating error for IToFP instruction " << I.getName() << "... ");

  return unOpErrorPassThrough(I);
}

bool InstructionPropagator::propagateFPToI(Instruction &I) {
  assert((isa<FPToSIInst>(I) || isa<FPToUIInst>(I)) && "Must be FPToI.");

  DEBUG(dbgs() << "Propagating error for FPToI instruction " << I.getName() << "... ");

  const AffineForm<inter_t> *Error = RMap.getError(I.getOperand(0U));
  if (Error == nullptr) {
    DEBUG(dbgs() << "ignored (no error data).\n");
    return false;
  }

  const FPInterval *Range = RMap.getRange(&I);
  if (Range == nullptr || Range->isUninitialized()) {
    DEBUG(dbgs() << "ignored (no range data).\n");
    return false;
  }

  AffineForm<inter_t> NewError = *Error + AffineForm<inter_t>(0.0, Range->getRoundingError());
  RMap.setError(&I, NewError);

  DEBUG(dbgs() << static_cast<double>(NewError.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool InstructionPropagator::propagateSelect(Instruction &I) {
  SelectInst &SI = cast<SelectInst>(I);

  DEBUG(dbgs() << "Propagating error for Select instruction " << I.getName() << "... ");

  if (RMap.getRangeError(&I) == nullptr) {
    DEBUG(dbgs() << "ignored (no range data).\n");
    return false;
  }

  auto *TV = getOperandRangeError(I, SI.getTrueValue());
  auto *FV = getOperandRangeError(I, SI.getFalseValue());
  if (TV == nullptr || !TV->second.hasValue()
      || FV == nullptr || !FV->second.hasValue()) {
    DEBUG(dbgs() << "(no data).\n");
    return false;
  }

  // Retrieve absolute errors attched to each value.
  inter_t ErrT = TV->second->noiseTermsAbsSum();
  inter_t ErrF = FV->second->noiseTermsAbsSum();

  // The error for this instruction is the maximum between the two branches.
  AffineForm<inter_t> ERes(0, std::max(ErrT, ErrF));

  // Add error to RMap.
  RMap.setError(&I, ERes);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, ERes);

  DEBUG(dbgs() << static_cast<double>(ERes.noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagatePhi(Instruction &I) {
  PHINode &PHI = cast<PHINode>(I);

  DEBUG(dbgs() << "Propagating error for PHI node " << I.getName() << "... ");

  const FPType *ConstFallbackTy = nullptr;
  if (RMap.getRangeError(&I) == nullptr) {
    for (const Use &IVal : PHI.incoming_values()) {
      auto *RE = getOperandRangeError(I, IVal);
      if (RE == nullptr)
	continue;

      ConstFallbackTy = dyn_cast_or_null<FPType>(RE->first.getTType());
      if (ConstFallbackTy != nullptr)
	break;
    }
  }

  // Iterate over values and choose the largest absolute error.
  inter_t AbsErr = -1.0;
  inter_t Min = std::numeric_limits<inter_t>::infinity();
  inter_t Max = -std::numeric_limits<inter_t>::infinity();
  for (const Use &IVal : PHI.incoming_values()) {
    auto *RE = getOperandRangeError(I, IVal, false, ConstFallbackTy);
    if (RE == nullptr)
      continue;

    Min = std::min(Min, RE->first.Min);
    Max = std::max(Max, RE->first.Max);

    if (!RE->second.hasValue())
      continue;

    AbsErr = std::max(AbsErr, RE->second->noiseTermsAbsSum());
  }

  if (AbsErr < 0.0) {
    // If no incoming value has an error, skip this instruction.
    DEBUG(dbgs() << "ignored (no error data).\n");
    return false;
  }

  AffineForm<inter_t> ERes(0, AbsErr);

  // Add error to RMap.
  if (RMap.getRangeError(&I) == nullptr) {
    FPInterval FPI(Interval<inter_t>(Min, Max));
    RMap.setRangeError(&I, std::make_pair(FPI, ERes));
  }
  else
    RMap.setError(&I, ERes);

  DEBUG(dbgs() << static_cast<double>(AbsErr) << ".\n");

  return true;
}

extern cl::opt<unsigned> CmpErrorThreshold;

bool InstructionPropagator::checkCmp(CmpErrorMap &CmpMap, Instruction &I) {
  CmpInst &CI = cast<CmpInst>(I);

  DEBUG(dbgs() << "Checking comparison error for ICmp/FCmp "
	<< CmpInst::getPredicateName(CI.getPredicate())
	<< " instruction " << I.getName() << "... ");

  auto *Op1 = getOperandRangeError(I, 0U);
  auto *Op2 = getOperandRangeError(I, 1U);
  if (Op1 == nullptr || Op1->first.isUninitialized() || !Op1->second.hasValue()
      || Op2 == nullptr || Op2->first.isUninitialized() || !Op2->second.hasValue()) {
    DEBUG(dbgs() << "(no data).\n");
    return false;
  }

  // Compute the total independent absolute error between operands.
  // (NoiseTerms present in both operands will cancel themselves.)
  inter_t AbsErr = (*Op1->second - *Op2->second).noiseTermsAbsSum();

  // Maximum absolute error tolerance to avoid changing the comparison result.
  inter_t MaxTol = std::max(computeMinRangeDiff(Op1->first, Op2->first),
			    Op1->first.getRoundingError());

  CmpErrorInfo CmpInfo(MaxTol, false);

  if (AbsErr >= MaxTol) {
    // The compare might be wrong due to the absolute error on operands.
    if (CmpErrorThreshold == 0) {
      CmpInfo.MayBeWrong = true;
    }
    else {
      // Check if it is also above custom threshold:
      inter_t RelErr = AbsErr / std::max(Op1->first.Max, Op2->first.Max);
      if (RelErr * 100.0 >= CmpErrorThreshold)
	CmpInfo.MayBeWrong = true;
    }
  }
  CmpMap.insert(std::make_pair(&I, CmpInfo));

  if (CmpInfo.MayBeWrong)
    DEBUG(dbgs() << "might be wrong!\n");
  else
    DEBUG(dbgs() << "no possible error.\n");

  return true;
}

bool InstructionPropagator::propagateRet(Instruction &I) {
  ReturnInst &RI = cast<ReturnInst>(I);

  DEBUG(dbgs() << "Propagating error for Return instruction "
	<< RI.getName() << "... ");

  // Get error of the returned value
  Value *Ret = RI.getReturnValue();
  const AffineForm<inter_t> *RetErr = RMap.getError(Ret);
  if (Ret == nullptr || RetErr == nullptr) {
    DEBUG(dbgs() << "unchanged (no data).\n");
    return false;
  }

  // Associate RetErr to the ret instruction.
  RMap.setError(&I, *RetErr);

  // Get error already associated to this function
  Function *F = RI.getFunction();
  const AffineForm<inter_t> *FunErr = RMap.getError(F);

  if (FunErr == nullptr
      || RetErr->noiseTermsAbsSum() > FunErr->noiseTermsAbsSum()) {
    // If no error had already been attached to this function,
    // or if RetErr is larger than the previous one, associate RetErr to it.
    RMap.setError(F, RetErr->flattenNoiseTerms());

    DEBUG(dbgs() << static_cast<double>(RetErr->noiseTermsAbsSum()) << ".\n");
    return true;
  }
  else {
    DEBUG(dbgs() << "unchanged (smaller than previous).\n");
    return true;
  }
}

bool InstructionPropagator::propagateCall(Instruction &I) {
  Function *F = nullptr;
  if (isa<CallInst>(I)) {
    F = cast<CallInst>(I).getCalledFunction();
    DEBUG(dbgs() << "Propagating error for Call instruction "
	  << I.getName() << "... ");
  }
  else {
    assert(isa<InvokeInst>(I));
    F = cast<InvokeInst>(I).getCalledFunction();
    DEBUG(dbgs() << "Propagating error for Invoke instruction "
	  << I.getName() << "... ");
  }

  if (F != nullptr && isSpecialFunction(*F)) {
    return propagateSpecialCall(I, *F);
  }

  if (RMap.getRangeError(&I) == nullptr) {
    DEBUG(dbgs() << "ignored (no range data).\n");
    return false;
  }

  const AffineForm<inter_t> *Error = RMap.getError(F);
  if (Error == nullptr) {
    DEBUG(dbgs() << "ignored (no error data).\n");
    return false;
  }

  RMap.setError(&I, *Error);

  DEBUG(dbgs() << static_cast<double>(Error->noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagateGetElementPtr(Instruction &I) {
  GetElementPtrInst &GEPI = cast<GetElementPtrInst>(I);

  DEBUG(dbgs() << "Propagating error for GetElementPtr instruction "
	<< GEPI.getName() << "... ");

  const RangeErrorMap::RangeError *RE =
    RMap.getRangeError(GEPI.getPointerOperand());
  if (RE == nullptr || !RE->second.hasValue()) {
    DEBUG(dbgs() << "ignored (no data).\n");
    return false;
  }

  RMap.setRangeError(&GEPI, *RE);

  DEBUG(dbgs() << static_cast<double>(RE->second->noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagateExtractValue(Instruction &I) {
  ExtractValueInst &EVI = cast<ExtractValueInst>(I);

  DEBUG(dbgs() << "Propagating error for ExtractValue instruction "
	<< I.getName() << "... ");

  const RangeErrorMap::RangeError *RE = RMap.getStructRangeError(&EVI);
  if (RE == nullptr || !RE->second.hasValue()) {
    DEBUG(dbgs() << "ignored (no data).\n");
    return false;
  }

  RMap.setRangeError(&EVI, *RE);

  DEBUG(dbgs() << static_cast<double>(RE->second->noiseTermsAbsSum()) << ".\n");

  return true;
}

bool InstructionPropagator::propagateInsertValue(Instruction &I) {
  InsertValueInst &IVI = cast<InsertValueInst>(I);

  DEBUG(dbgs() << "Propagating error for ExtractValue instruction "
	<< I.getName() << "... ");


  const RangeErrorMap::RangeError *RE =
    RMap.getStructRangeError(IVI.getInsertedValueOperand());
  if (RE == nullptr || !RE->second.hasValue()) {
    DEBUG(dbgs() << "ignored (no data).\n");
    return false;
  }

  RMap.setStructRangeError(&IVI, *RE);

  DEBUG(dbgs() << static_cast<double>(RE->second->noiseTermsAbsSum()) << ".\n");

  return false;
}

} // end of namespace ErrorProp

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

#include <cassert>
#include "llvm/Support/Debug.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Argument.h"
#include "llvm/Transforms/ErrorPropagator/AffineForms.h"
#include "llvm/Transforms/ErrorPropagator/Metadata.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

namespace {

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
	     const FPInterval &R2, const AffineForm<inter_t> &E2) {
  // Compute y / z as y * 1/z.

  // Compute the range of 1/z.
  FPInterval InvR2(R2);
  InvR2.Min = 1.0 / R2.Max;
  InvR2.Max = 1.0 / R2.Min;

  // Compute errors on 1/z.
  AffineForm<inter_t> E1OverZ = (AffineForm<inter_t>(R2) + E2).inverse(true);
  // Discard central value, only keep errors.
  E1OverZ.setCentralValue(0.0);

  // The error for y / z will be
  // x * err1/z + 1/z * errx + errx * err1/z
  // plus the rounding error due to truncation.
  return AffineForm<inter_t>(R1) * E1OverZ
    + AffineForm<inter_t>(InvR2) * E1
    + E1 * E1OverZ
    + AffineForm<inter_t>(0, R1.getRoundingError());
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

const std::pair<FPInterval, AffineForm<inter_t> >*
getOperandRangeError(RangeErrorMap &RMap, Instruction &I, Value *V) {
  assert(V != nullptr);

  // First, check if Range and Error have already been computed.
  auto *RE = RMap.getRangeError(V);
  if (RE != nullptr)
    return RE;

  // If V is a Constant we extract its value.
  ConstantInt *VInt = dyn_cast<ConstantInt>(V);
  if (VInt == nullptr)
    return nullptr;

  // We interpret its value with the same fractional bits and sign of the result.
  const FPInterval *RInfo = RMap.getRange(&I);
  if (RInfo == nullptr)
    return nullptr;

  std::unique_ptr<FixedPointValue> VFPRange =
    FixedPointValue::createFromConstantInt(RInfo->getSPointPos(),
					   nullptr, VInt, VInt);
  FPInterval VRange = VFPRange->getInterval();

  // We use the rounding error of this format as the only error.
  RMap.setRangeError(V,
		     std::make_pair(VRange,
				    AffineForm<inter_t>(0, VRange.getRoundingError())));
  return RMap.getRangeError(V);
}

const std::pair<FPInterval, AffineForm<inter_t> >*
getOperandRangeError(RangeErrorMap &RMap, Instruction &I, unsigned Op) {
  Value *V = I.getOperand(Op);
  if (V == nullptr)
    return nullptr;

  return getOperandRangeError(RMap, I, V);
}

} // end of anonymous namespace

void propagateBinaryOp(RangeErrorMap &RMap, Instruction &I) {
  BinaryOperator &BI = cast<BinaryOperator>(I);

  DEBUG(dbgs() << "Computing error for " << BI.getOpcodeName()
	<< " instruction " << I.getName() << "... ");

  auto *O1 = getOperandRangeError(RMap, BI, 0U);
  auto *O2 = getOperandRangeError(RMap, BI, 1U);
  if (O1 == nullptr || O2 == nullptr) {
    DEBUG(dbgs() << "no data.\n");
    return;
  }

  AffineForm<inter_t> ERes;
  switch(BI.getOpcode()) {
    case Instruction::Add:
      ERes = propagateAdd(O1->first, O1->second,
			  O2->first, O2->second);
      break;
    case Instruction::Sub:
      ERes = propagateSub(O1->first, O1->second,
			  O2->first, O2->second);
      break;
    case Instruction::Mul:
      ERes = propagateMul(O1->first, O1->second,
			  O2->first, O2->second);
      break;
    case Instruction::UDiv:
      // Fall-through.
    case Instruction::SDiv:
      ERes = propagateDiv(O1->first, O1->second,
			  O2->first, O2->second);
      break;
    case Instruction::Shl:
      ERes = propagateShl(O1->second);
      break;
    case Instruction::LShr:
      // Fall-through.
    case Instruction::AShr: {
      const FPInterval *ResR = RMap.getRange(&I);
      if (ResR == nullptr) {
	DEBUG(dbgs() << "no data.");
	return;
      }
      ERes = propagateShr(O1->second, *ResR);
      break;
    }
    default:
      DEBUG(dbgs() << "not supported.\n");
      return;
  }

  // Add error to RMap.
  RMap.setError(&BI, ERes);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, ERes);

  DEBUG(dbgs() << static_cast<double>(ERes.noiseTermsAbsSum()) << ".\n");
}

void propagateStore(RangeErrorMap &RMap, Instruction &I) {
  assert(I.getOpcode() == Instruction::Store && "Must be Store.");
  StoreInst &SI = cast<StoreInst>(I);

  DEBUG(dbgs() << "Propagating error for Store instruction " << I.getName() << "... ");

  auto *SrcRE = getOperandRangeError(RMap, I, 0U);
  if (SrcRE == nullptr) {
    DEBUG(dbgs() << " ignored (no data).\n");
    return;
  }

  Value *IDest = SI.getPointerOperand();
  assert(IDest != nullptr && "Store with null Pointer Operand.\n");

  // Associate the propagated error to the pointer instruction.
  RangeErrorMap::RangeError SrcRECopy = *SrcRE;
  RMap.setRangeError(IDest, SrcRECopy);

  DEBUG(dbgs() << static_cast<double>(SrcRECopy.second.noiseTermsAbsSum()) << ".\n");
}

void propagateLoad(RangeErrorMap &RMap, Instruction &I) {
  assert(I.getOpcode() == Instruction::Load && "Must be Load.");
  LoadInst &LI = cast<LoadInst>(I);

  DEBUG(dbgs() << "Propagating error for Load instruction " << I.getName() << "... ");

  Value *VSrc = LI.getPointerOperand();
  const RangeErrorMap::RangeError *SrcRE =
    RMap.getRangeError(VSrc);
  if (SrcRE == nullptr) {
    // There is no data associated with this source address
    // We use metadata, if there are any
    const FPInterval *SrcR = RMap.getRange(&I);
    if (SrcR == nullptr) {
      DEBUG(dbgs() << " ignored (no metadata).\n");
      return;
    }
    // We just add the rounding error
    AffineForm<inter_t> RErr(0, SrcR->getRoundingError());
    RMap.setError(&I, RErr);

    DEBUG(dbgs() << static_cast<double>(RErr.noiseTermsAbsSum()) << ".\n");
    return;
  }
  // If there are range and error associated
  // to the source address we propagate them.
  // (We need to copy it because the pointer is invalidated by setRangeError.)
  RangeErrorMap::RangeError RECopy = *SrcRE;
  RMap.setRangeError(&I, RECopy);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, SrcRE->second);

  DEBUG(dbgs() << static_cast<double>(RECopy.second.noiseTermsAbsSum()) << ".\n");
}

void unOpErrorPassThrough(RangeErrorMap &RMap, Instruction &I) {
  assert(isa<UnaryInstruction>(I) && "Must be Unary.");

  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr) {
    DEBUG(dbgs() << "no data.\n");
    return;
  }

  // Add error to RMap.
  AffineForm<inter_t> OpErr = OpRE->second;
  RMap.setError(&I, OpErr);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, OpRE->second);

  DEBUG(dbgs() << static_cast<double>(OpErr.noiseTermsAbsSum()) << ".\n");
}

void propagateIExt(RangeErrorMap &RMap, Instruction &I) {
  assert((I.getOpcode() == Instruction::SExt
	  || I.getOpcode() == Instruction::ZExt) && "Must be SExt or ZExt.");

  DEBUG(dbgs() << "Propagating error for Int Extend instruction " << I.getName() << "... ");

  // No further error is introduced with signed/unsigned extension.
  unOpErrorPassThrough(RMap, I);
}

void propagateTrunc(RangeErrorMap &RMap, Instruction &I) {
  assert(I.getOpcode() == Instruction::Trunc && "Must be Trunc.");

  DEBUG(dbgs() << "Propagating error for Trunc instruction " << I.getName() << "... ");

  // No further error is introduced with truncation if no overflow occurs
  // (in which case it is useless to propagate other errors).
  unOpErrorPassThrough(RMap, I);
}

void propagateSelect(RangeErrorMap &RMap, Instruction &I) {
  SelectInst &SI = cast<SelectInst>(I);

  DEBUG(dbgs() << "Propagating error for Select instruction " << I.getName() << "... ");

  auto *TV = getOperandRangeError(RMap, I, SI.getTrueValue());
  auto *FV = getOperandRangeError(RMap, I, SI.getFalseValue());
  if (TV == nullptr || FV == nullptr) {
    DEBUG(dbgs() << "(no data).\n");
    return;
  }

  // Retrieve absolute errors attched to each value.
  inter_t ErrT = TV->second.noiseTermsAbsSum();
  inter_t ErrF = FV->second.noiseTermsAbsSum();

  // The error for this instruction is the maximum between the two branches.
  AffineForm<inter_t> ERes(0, std::max(ErrT, ErrF));

  // Add error to RMap.
  RMap.setError(&I, ERes);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, ERes);

  DEBUG(dbgs() << static_cast<double>(ERes.noiseTermsAbsSum()) << ".\n");
}

void propagatePhi(RangeErrorMap &RMap, Instruction &I) {
  PHINode &PHI = cast<PHINode>(I);

  DEBUG(dbgs() << "Propagating error for PHI node " << I.getName() << "... ");

  // Iterate over values and choose the largest absolute error.
  inter_t AbsErr = 0.0;
  for (const Use &IVal : PHI.incoming_values()) {
    auto *RE = getOperandRangeError(RMap, I, IVal);
    if (RE == nullptr) {
      continue;
    }
    AbsErr = std::max(AbsErr, RE->second.noiseTermsAbsSum());
  }

  AffineForm<inter_t> ERes(0, AbsErr);

  // Add error to RMap.
  RMap.setError(&I, ERes);

  // Add computed error metadata to the instruction.
  // setErrorMetadata(I, ERes);

  DEBUG(dbgs() << static_cast<double>(AbsErr) << ".\n");
}

inter_t computeMinRangeDiff(const FPInterval &R1, const FPInterval &R2) {
  // Check if the two ranges overlap.
  if (R1.Min <= R2.Max && R2.Min <= R1.Max) {
    return 0.0;
  }

  // Otherwise either R1 < R2 or R2 < R1.
  if (R1.Max < R2.Min) {
    // R1 < R2
    return R2.Min - R1.Max;
  }

  // Else R2 < R1
  assert(R2.Max < R1.Min);
  return R1.Min - R2.Max;
}

extern cl::opt<unsigned> CmpErrorThreshold;

void checkICmp(RangeErrorMap &RMap, CmpErrorMap &CmpMap, Instruction &I) {
  ICmpInst &CI = cast<ICmpInst>(I);

  DEBUG(dbgs() << "Checking comparison error for ICmp "
	<< CmpInst::getPredicateName(CI.getPredicate())
	<< " instruction " << I.getName() << "... ");

  auto *Op1 = getOperandRangeError(RMap, I, 0U);
  auto *Op2 = getOperandRangeError(RMap, I, 1U);
  if (Op1 == nullptr || Op2 == nullptr) {
    DEBUG(dbgs() << "(no data).\n");
    return;
  }

  // Compute the total independent absolute error between operands.
  // (NoiseTerms present in both operands will cancel themselves.)
  inter_t AbsErr = (Op1->second - Op2->second).noiseTermsAbsSum();

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
}

void propagateRet(RangeErrorMap &RMap, Instruction &I) {
  ReturnInst &RI = cast<ReturnInst>(I);

  DEBUG(dbgs() << "Propagating error for Return instruction "
	<< RI.getName() << "... ");

  // Get error of the returned value
  Value *Ret = RI.getReturnValue();
  const AffineForm<inter_t> *RetErr = RMap.getError(Ret);
  if (Ret == nullptr || RetErr == nullptr) {
    DEBUG(dbgs() << "unchanged (no data).\n");
    return;
  }
  AffineForm<inter_t> RetErrCopy = *RetErr;
  // Associate RetErr to the ret instruction.
  RMap.setError(&I, RetErrCopy);

  // Get error already associated to this function
  Function *F = RI.getFunction();
  const AffineForm<inter_t> *FunErr = RMap.getError(F);

  if (FunErr == nullptr
      || RetErrCopy.noiseTermsAbsSum() > FunErr->noiseTermsAbsSum()) {
    // If no error had already been attached to this function,
    // or if RetErr is larger than the previous one, associate RetErr to it.
    RMap.setError(F, RetErrCopy.flattenNoiseTerms());

    DEBUG(dbgs() << static_cast<double>(RetErrCopy.noiseTermsAbsSum()) << ".\n");
  }
  else {
    DEBUG(dbgs() << "unchanged (smaller than previous).\n");
  }
}

void propagateCall(RangeErrorMap &RMap, Instruction &I) {
  CallInst &CI = cast<CallInst>(I);

  DEBUG(dbgs() << "Propagating error for Call instruction "
	<< CI.getName() << "... ");

  Value *F = CI.getCalledValue();
  const AffineForm<inter_t> *Error = RMap.getError(F);
  AffineForm<inter_t> ErrorCopy(0U);
  if (Error == nullptr) {
    DEBUG(dbgs() << "(no data), ");
  }
  else {
    ErrorCopy = *Error;
  }

  RMap.setError(&I, ErrorCopy);

  DEBUG(dbgs() << static_cast<double>(ErrorCopy.noiseTermsAbsSum()) << ".\n");
}

} // end of namespace ErrorProp

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

#include "ErrorPropagator/Propagators.h"

#include <cassert>
#include <algorithm>
#include <cmath>
#include "llvm/Support/Debug.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Argument.h"
#include "ErrorPropagator/AffineForms.h"
#include "ErrorPropagator/MDUtils/Metadata.h"

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

const RangeErrorMap::RangeError *
getConstantFPRangeError(RangeErrorMap &RMap, ConstantFP *VFP) {
  double CVal;
  if (VFP->getType()->isDoubleTy())
    CVal = VFP->getValueAPF().convertToDouble();
  else if (VFP->getType()->isFloatTy())
    CVal = VFP->getValueAPF().convertToFloat();
  else
    return nullptr;

  // Kludge! ;-)
  CVal = 1.0;

  FPInterval FPI(Interval<inter_t>(CVal, CVal));
  RMap.setRangeError(VFP, std::make_pair(FPI, AffineForm<inter_t>(0.0)));
  return RMap.getRangeError(VFP);
}

const RangeErrorMap::RangeError *
getConstantRangeError(RangeErrorMap &RMap, Instruction &I, ConstantInt *VInt,
		      bool DoublePP = false) {

  // We interpret the value of VInt with the same
  // fractional bits and sign of the result.
  const FPInterval *RInfo = RMap.getRange(&I);
  if (RInfo == nullptr)
    return nullptr;

  const FPType *Ty = cast<FPType>(RInfo->getTType());
  unsigned PointPos = Ty->getPointPos();
  if (DoublePP) PointPos *= 2U;
  int SPointPos = (Ty->isSigned()) ? -PointPos : PointPos;
  std::unique_ptr<FixedPointValue> VFPRange =
    FixedPointValue::createFromConstantInt(SPointPos, nullptr, VInt, VInt);
  FPInterval VRange = VFPRange->getInterval();

#if 1
  // We use the rounding error of this format as the only error.
  AffineForm<inter_t> Error(0, RInfo->getRoundingError());
#else
  AffineForm<inter_t> Error;
#endif

  RMap.setRangeError(VInt, std::make_pair(VRange, Error));
  return RMap.getRangeError(VInt);
}

const RangeErrorMap::RangeError*
getOperandRangeError(RangeErrorMap &RMap, Instruction &I, Value *V, bool DoublePP = false) {
  assert(V != nullptr);

  // If V is a Constant Int extract its value.
  ConstantInt *VInt = dyn_cast<ConstantInt>(V);
  if (VInt != nullptr)
    return getConstantRangeError(RMap, I, VInt, DoublePP);

  ConstantFP *VFP = dyn_cast<ConstantFP>(V);
  if (VFP != nullptr)
    return getConstantFPRangeError(RMap, VFP);

  // Otherwise, check if Range and Error have already been computed.
  return RMap.getRangeError(V);
}

const RangeErrorMap::RangeError*
getOperandRangeError(RangeErrorMap &RMap, Instruction &I, unsigned Op, bool DoublePP = false) {
  Value *V = I.getOperand(Op);
  if (V == nullptr)
    return nullptr;

  return getOperandRangeError(RMap, I, V, DoublePP);
}

} // end of anonymous namespace

bool propagateBinaryOp(RangeErrorMap &RMap, Instruction &I) {
  BinaryOperator &BI = cast<BinaryOperator>(I);

  DEBUG(dbgs() << "Computing error for " << BI.getOpcodeName()
	<< " instruction " << I.getName() << "... ");

  // if (RMap.getRangeError(&I) == nullptr) {
  //   DEBUG(dbgs() << "ignored (no range data).\n");
  //   return false;
  // }

  bool DoublePP = BI.getOpcode() == Instruction::UDiv
    || BI.getOpcode() == Instruction::SDiv;
  auto *O1 = getOperandRangeError(RMap, BI, 0U, DoublePP);
  auto *O2 = getOperandRangeError(RMap, BI, 1U);
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

void updateArgumentRE(RangeErrorMap &RMap, MemorySSA &MemSSA, Value *Pointer,
		      const RangeErrorMap::RangeError *NewRE) {
  assert(Pointer != nullptr);
  assert(NewRE != nullptr);

  if (isa<Argument>(Pointer)) {
    auto *PointerRE = RMap.getRangeError(Pointer);
    if (PointerRE == nullptr || !PointerRE->second.hasValue()
	|| PointerRE->second->noiseTermsAbsSum() < NewRE->second->noiseTermsAbsSum()) {
      RMap.setRangeError(Pointer, *NewRE);
      DEBUG(dbgs() << "(Error of argument "<< Pointer->getName() << " updated.) ");
    }
  }
  else if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(Pointer)) {
    updateArgumentRE(RMap, MemSSA, GEPI->getPointerOperand(), NewRE);
  }
  else if (LoadInst *LI = dyn_cast<LoadInst>(Pointer)) {
    MemorySSAWalker *MSSAWalker = MemSSA.getWalker();
    assert(MSSAWalker != nullptr && "Null MemorySSAWalker.");
    if (MemoryDef *MD = dyn_cast<MemoryDef>(MSSAWalker->getClobberingMemoryAccess(LI))) {
      if (StoreInst *SI = dyn_cast<StoreInst>(MD->getMemoryInst())) {
	updateArgumentRE(RMap, MemSSA, SI->getValueOperand(), NewRE);
      }
    }
    // TODO: Handle MemoryPHI
  }
}

bool propagateStore(RangeErrorMap &RMap, MemorySSA &MemSSA, Instruction &I) {
  assert(I.getOpcode() == Instruction::Store && "Must be Store.");
  StoreInst &SI = cast<StoreInst>(I);

  DEBUG(dbgs() << "Propagating error for Store instruction " << I.getName() << "... ");

  Value *IDest = SI.getPointerOperand();
  assert(IDest != nullptr && "Store with null Pointer Operand.\n");
  auto *PointerRE = RMap.getRangeError(IDest);

  auto *SrcRE = getOperandRangeError(RMap, I, 0U);
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
  updateArgumentRE(RMap, MemSSA, IDest, SrcRE);

  DEBUG(dbgs() << static_cast<double>(SrcRE->second->noiseTermsAbsSum()) << ".\n");

  // Update struct errors.
  RMap.updateStructElemError(SI, SrcRE->second.getPointer());

  return true;
}

void findLOEError(RangeErrorMap &RMap, Instruction *I,
		  SmallVectorImpl<const RangeErrorMap::RangeError *> &Res) {
  Value *Pointer;
  switch(I->getOpcode()) {
    case Instruction::Load:
      Pointer = (cast<LoadInst>(I))->getPointerOperand();
      break;
    case Instruction::GetElementPtr:
      Pointer = (cast<GetElementPtrInst>(I))->getPointerOperand();
      break;
    default:
      return;
  }
  const RangeErrorMap::RangeError *RE = RMap.getRangeError(Pointer);
  if (RE != nullptr)
    Res.push_back(RE);
  else {
    Instruction *PI = dyn_cast<Instruction>(Pointer);
    if (PI != nullptr)
      findLOEError(RMap, PI, Res);
  }
}

void findMemDefError(RangeErrorMap &RMap, Instruction *I, const MemoryDef *MD,
		     SmallVectorImpl<const RangeErrorMap::RangeError *> &Res) {
  assert(MD != nullptr && "MD is null.");

  Instruction *MI = MD->getMemoryInst();
  if (isa<CallInst>(MI) || isa<InvokeInst>(MI))
    // The computed error should have been attached to the actual parameter.
    findLOEError(RMap, I, Res);
  else
    Res.push_back(RMap.getRangeError(MD->getMemoryInst()));
}

void findMemPhiError(RangeErrorMap &RMap,
		     MemorySSA &MemSSA,
		     Instruction *I,
		     MemoryPhi *MPhi,
		     SmallSet<MemoryAccess *, DEFAULT_RE_COUNT> &Visited,
		     SmallVectorImpl<const RangeErrorMap::RangeError *> &Res) {
  assert(MPhi != nullptr && "MPhi is null.");

  for (Use &MU : MPhi->incoming_values()) {
    MemoryAccess *MA = cast<MemoryAccess>(&MU);
    findMemSSAError(RMap, MemSSA, I, MA, Visited, Res);
  }
}

void findMemSSAError(RangeErrorMap &RMap, MemorySSA &MemSSA,
		     Instruction *I, MemoryAccess *MA,
		     SmallSet<MemoryAccess *, DEFAULT_RE_COUNT> &Visited,
		     SmallVectorImpl<const RangeErrorMap::RangeError *> &Res) {
  if (MA == nullptr) {
    DEBUG(dbgs() << "WARNING: nullptr MemoryAccess passed to findMemSSAError!\n");
    return;
  }

  if (!Visited.insert(MA).second)
    return;

  if (MemSSA.isLiveOnEntryDef(MA))
    findLOEError(RMap, I, Res);
  else if (isa<MemoryUse>(MA)) {
    MemorySSAWalker *MSSAWalker = MemSSA.getWalker();
    assert(MSSAWalker != nullptr && "Null MemorySSAWalker.");
    findMemSSAError(RMap, MemSSA, I,
		    MSSAWalker->getClobberingMemoryAccess(MA),
		    Visited, Res);
  }
  else if (isa<MemoryDef>(MA))
    findMemDefError(RMap, I, cast<MemoryDef>(MA), Res);
  else {
    assert(isa<MemoryPhi>(MA));
    findMemPhiError(RMap, MemSSA, I, cast<MemoryPhi>(MA), Visited, Res);
  }
}

bool propagateLoad(RangeErrorMap &RMap, MemorySSA &MemSSA, Instruction &I) {
  assert(I.getOpcode() == Instruction::Load && "Must be Load.");
  LoadInst &LI = cast<LoadInst>(I);

  DEBUG(dbgs() << "Propagating error for Load instruction " << I.getName() << "... ");

  // Look for range and error in the defining instructions with MemorySSA
  SmallSet<MemoryAccess *, DEFAULT_RE_COUNT> Visited;
  SmallVector<const RangeErrorMap::RangeError *, DEFAULT_RE_COUNT> REs;
  findMemSSAError(RMap, MemSSA, &I, MemSSA.getMemoryAccess(&I), Visited, REs);

  // Kludje for when AliasAnalysis fails (i.e. almost always).
  // findLOEError(RMap, &I, REs);

  std::sort(REs.begin(), REs.end());
  auto NewEnd = std::unique(REs.begin(), REs.end());
  REs.resize(NewEnd - REs.begin());

  if (REs.size() == 1U && REs.front() != nullptr) {
    // If we found only one defining instruction, we just use its data.
    const RangeErrorMap::RangeError *RE = REs.front();
    RMap.setRangeError(&I, *RE);
    DEBUG(if (RE->second.hasValue())
	    dbgs() << static_cast<double>(RE->second->noiseTermsAbsSum()) << ".\n";
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
  DEBUG(dbgs() << static_cast<double>(Error.noiseTermsAbsSum()) << ".\n");

  return true;
}

bool unOpErrorPassThrough(RangeErrorMap &RMap, Instruction &I) {
  // assert(isa<UnaryInstruction>(I) && "Must be Unary.");

  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  auto *DestRE = RMap.getRangeError(&I);
  if (DestRE == nullptr || DestRE->first.isUninitialized()) {
    // Add operand range and error to RMap.
    RMap.setRangeError(&I, *OpRE);
  }
  else {
    // Add only error to RMap.
    RMap.setError(&I, *OpRE->second);
  }

  DEBUG(dbgs() << static_cast<double>(OpRE->second->noiseTermsAbsSum()) << ".\n");

  return true;
}

bool propagateExt(RangeErrorMap &RMap, Instruction &I) {
  assert((I.getOpcode() == Instruction::SExt
	  || I.getOpcode() == Instruction::ZExt
	  || I.getOpcode() == Instruction::FPExt)
	 && "Must be SExt, ZExt or FExt.");

  DEBUG(dbgs() << "Propagating error for Extend instruction " << I.getName() << "... ");

  // No further error is introduced with signed/unsigned extension.
  return unOpErrorPassThrough(RMap, I);
}

bool propagateTrunc(RangeErrorMap &RMap, Instruction &I) {
  assert((I.getOpcode() == Instruction::Trunc
	  || I.getOpcode() == Instruction::FPTrunc)
	 && "Must be Trunc.");

  DEBUG(dbgs() << "Propagating error for Trunc instruction " << I.getName() << "... ");

  // No further error is introduced with truncation if no overflow occurs
  // (in which case it is useless to propagate other errors).
  return unOpErrorPassThrough(RMap, I);
}

bool propagateIToFP(RangeErrorMap &RMap, Instruction &I) {
  assert((isa<SIToFPInst>(I) || isa<UIToFPInst>(I)) && "Must be IToFP.");

  DEBUG(dbgs() << "Propagating error for IToFP instruction " << I.getName() << "... ");

  return unOpErrorPassThrough(RMap, I);
}

bool propagateFPToI(RangeErrorMap &RMap, Instruction &I) {
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

bool propagateSelect(RangeErrorMap &RMap, Instruction &I) {
  SelectInst &SI = cast<SelectInst>(I);

  DEBUG(dbgs() << "Propagating error for Select instruction " << I.getName() << "... ");

  if (RMap.getRangeError(&I) == nullptr) {
    DEBUG(dbgs() << "ignored (no range data).\n");
    return false;
  }

  auto *TV = getOperandRangeError(RMap, I, SI.getTrueValue());
  auto *FV = getOperandRangeError(RMap, I, SI.getFalseValue());
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

bool propagatePhi(RangeErrorMap &RMap, Instruction &I) {
  PHINode &PHI = cast<PHINode>(I);

  DEBUG(dbgs() << "Propagating error for PHI node " << I.getName() << "... ");

  // if (RMap.getRangeError(&I) == nullptr) {
  //   DEBUG(dbgs() << "ignored (no range data).\n");
  //   return false;
  // }

  // Iterate over values and choose the largest absolute error.
  inter_t AbsErr = -1.0;
  inter_t Min = std::numeric_limits<inter_t>::infinity();
  inter_t Max = -std::numeric_limits<inter_t>::infinity();
  for (const Use &IVal : PHI.incoming_values()) {
    auto *RE = getOperandRangeError(RMap, I, IVal);
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

bool checkCmp(RangeErrorMap &RMap, CmpErrorMap &CmpMap, Instruction &I) {
  CmpInst &CI = cast<CmpInst>(I);

  DEBUG(dbgs() << "Checking comparison error for ICmp/FCmp "
	<< CmpInst::getPredicateName(CI.getPredicate())
	<< " instruction " << I.getName() << "... ");

  auto *Op1 = getOperandRangeError(RMap, I, 0U);
  auto *Op2 = getOperandRangeError(RMap, I, 1U);
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

bool propagateRet(RangeErrorMap &RMap, Instruction &I) {
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

bool isSqrt(Function &F) {
  return F.getName() == "sqrtf"
    || F.getName() == "sqrt"
    || F.getName() == "_ZSt4sqrtf"
    || F.getName() == "_ZSt4sqrtf_fixp";
}

bool isLog(Function &F) {
  return F.getName() == "log"
    || F.getName() == "logf"
    || F.getName() == "_ZSt3logf"
    || F.getName() == "_ZSt3logf_fixp";
}

bool isExp(Function &F) {
  return F.getName() == "expf"
    || F.getName() == "exp"
    || F.getName() == "_ZSt3expf"
    || F.getName() == "_ZSt3expf_fixp";
}

bool isAcos(Function &F) {
  return F.getName() == "acos"
    || F.getName() == "acosf";
}

bool isAsin(Function &F) {
  return F.getName() == "asin"
    || F.getName() == "asinf";
}

bool isSpecialFunction(Function &F) {
  return F.arg_size() == 1U
    && (F.empty() || !F.hasName()
	|| isSqrt(F) || isLog(F) || isExp(F) || isAcos(F) || isAsin(F));
}

bool propagateSqrt(RangeErrorMap &RMap, Instruction &I) {
  DEBUG(dbgs() << "(special: sqrt) ");
  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> NewErr =
    LinearErrorApproximationDecr([](inter_t x){ return static_cast<inter_t>(0.5) / std::sqrt(x); },
				 OpRE->first, OpRE->second.getValue())
    + AffineForm<inter_t>(0.0, OpRE->first.getRoundingError());

  RMap.setError(&I, NewErr);

  DEBUG(dbgs() << static_cast<double>(NewErr.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool propagateLog(RangeErrorMap &RMap, Instruction &I) {
  DEBUG(dbgs() << "(special: log) ");
  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> NewErr =
    LinearErrorApproximationDecr([](inter_t x){ return static_cast<inter_t>(1) / x; },
				 OpRE->first, OpRE->second.getValue())
    + AffineForm<inter_t>(0.0, OpRE->first.getRoundingError());

  RMap.setError(&I, NewErr);

  DEBUG(dbgs() << static_cast<double>(NewErr.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool propagateExp(RangeErrorMap &RMap, Instruction &I) {
  DEBUG(dbgs() << "(special: exp) ");
  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> NewErr =
    LinearErrorApproximationIncr([](inter_t x){ return std::exp(x); },
				 OpRE->first, OpRE->second.getValue())
    + AffineForm<inter_t>(0.0, OpRE->first.getRoundingError());

  RMap.setError(&I, NewErr);

  DEBUG(dbgs() << static_cast<double>(NewErr.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool propagateAcos(RangeErrorMap &RMap, Instruction &I) {
  DEBUG(dbgs() << "(special: acos) ");
  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> NewErr =
    LinearErrorApproximationIncr([](inter_t x){ return static_cast<inter_t>(-1) / std::sqrt(1 - x*x); },
				 OpRE->first, OpRE->second.getValue())
    + AffineForm<inter_t>(0.0, OpRE->first.getRoundingError());

  RMap.setError(&I, NewErr);

  DEBUG(dbgs() << static_cast<double>(NewErr.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool propagateAsin(RangeErrorMap &RMap, Instruction &I) {
  DEBUG(dbgs() << "(special: asin) ");
  auto *OpRE = getOperandRangeError(RMap, I, 0U);
  if (OpRE == nullptr || !OpRE->second.hasValue()) {
    DEBUG(dbgs() << "no data.\n");
    return false;
  }

  AffineForm<inter_t> NewErr =
    LinearErrorApproximationIncr([](inter_t x){ return static_cast<inter_t>(1) / std::sqrt(1 - x*x); },
				 OpRE->first, OpRE->second.getValue())
    + AffineForm<inter_t>(0.0, OpRE->first.getRoundingError());

  RMap.setError(&I, NewErr);

  DEBUG(dbgs() << static_cast<double>(NewErr.noiseTermsAbsSum()) << ".\n");
  return true;
}

bool propagateSpecialCall(RangeErrorMap &RMap, Instruction &I, Function &Called) {
  assert(isSpecialFunction(Called));
  if (isSqrt(Called)) {
    return propagateSqrt(RMap, I);
  }
  else if (isLog(Called)) {
    return propagateLog(RMap, I);
  }
  else if (isExp(Called)) {
    return propagateExp(RMap, I);
  }
  else if (isAcos(Called)) {
    return propagateAcos(RMap, I);
  }
  else if (isAsin(Called)) {
    return propagateAsin(RMap, I);
  }
  else {
    DEBUG(dbgs() << "(special pass-through) ");
    return unOpErrorPassThrough(RMap, I);
  }
}

bool propagateCall(RangeErrorMap &RMap, Instruction &I) {
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
    return propagateSpecialCall(RMap, I, *F);
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

bool propagateGetElementPtr(RangeErrorMap &RMap, Instruction &I) {
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

} // end of namespace ErrorProp

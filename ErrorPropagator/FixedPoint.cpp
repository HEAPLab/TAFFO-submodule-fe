//===-- FixedPoint.cpp - Representation of fixed-point values ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementations of
/// fixed point arithmetic type wrappers.
///
//===----------------------------------------------------------------------===//

#include <cmath>

#include "llvm/Support/ErrorHandling.h"
#include "ErrorPropagator/FixedPoint.h"

namespace ErrorProp {

namespace {

ConstantAsMetadata *getBoundMetadata(LLVMContext &C, uint32_t Bound) {
  IntegerType *BoundType = IntegerType::get(C, 32U);
  ConstantInt *BoundCInt = ConstantInt::get(BoundType, Bound);
  return ConstantAsMetadata::get(BoundCInt);
}

ConstantAsMetadata *getBoundMetadata(LLVMContext &C, uint64_t Bound) {
  IntegerType *BoundType = IntegerType::get(C, 64U);
  ConstantInt *BoundCInt = ConstantInt::get(BoundType, Bound);
  return ConstantAsMetadata::get(BoundCInt);
}

ConstantAsMetadata *getBoundMetadata(LLVMContext &C, int32_t Bound) {
  IntegerType *BoundType = IntegerType::get(C, 32U);
  ConstantInt *BoundCInt = ConstantInt::getSigned(BoundType, Bound);
  return ConstantAsMetadata::get(BoundCInt);
}

ConstantAsMetadata *getBoundMetadata(LLVMContext &C, int64_t Bound) {
  IntegerType *BoundType = IntegerType::get(C, 64U);
  ConstantInt *BoundCInt = ConstantInt::getSigned(BoundType, Bound);
  return ConstantAsMetadata::get(BoundCInt);
}

const ConstantInt *getConstantIntMDOperand(const MDNode &N, unsigned I) {
   const ConstantAsMetadata *CMD =
    dyn_cast_or_null<ConstantAsMetadata>(N.getOperand(I).get());
   if (CMD == nullptr)
     return nullptr;

   return dyn_cast_or_null<ConstantInt>(CMD->getValue());
}

} // end anonymous namespace

std::unique_ptr<FixedPointValue>
FixedPointValue::createFromMDNode(const IntegerType *IT, const MDNode &N) {
  if (N.getNumOperands() < 3)
    return nullptr;

  // Get sign and precision (signed if negative precision)
  const ConstantInt *CSPrec = getConstantIntMDOperand(N, 0U);
  if (CSPrec == nullptr)
    return nullptr;

  int64_t SPrec = CSPrec->getSExtValue();

  // Get range bounds
  const ConstantInt *CIMin = getConstantIntMDOperand(N, 1U);
  const ConstantInt *CIMax = getConstantIntMDOperand(N, 2U);

  return FixedPointValue::createFromConstantInt(SPrec, IT, CIMin, CIMax);
}

std::unique_ptr<FixedPointValue>
FixedPointValue::createFromConstantInt(int SPrec,
				       const IntegerType *IT,
				       const ConstantInt *CIMin,
				       const ConstantInt *CIMax) {
  if (CIMin == nullptr || CIMax == nullptr)
    return nullptr;

  if (IT == nullptr) {
    // If there is no Instruction IntegerType, we get it from the operands.
    IT = CIMin->getType();
  }
  if (SPrec >= 0) {
    if (IT->getBitWidth() <= 32U) {
      return std::unique_ptr<FixedPointValue>
	(new UFixedPoint32(SPrec,
			   CIMin->getZExtValue(),
			   CIMax->getZExtValue()));
    }
    if (IT->getBitWidth() <= 64U) {
      return std::unique_ptr<FixedPointValue>
	(new UFixedPoint64(SPrec,
			   CIMin->getZExtValue(),
			   CIMax->getZExtValue()));
    }
    llvm_unreachable("Types wider than 64 bits not supported.");
  }
  else {
    SPrec = -SPrec;
    if (IT->getBitWidth() <= 32U) {
      return std::unique_ptr<FixedPointValue>
	(new SFixedPoint32(SPrec,
			   CIMin->getSExtValue(),
			   CIMax->getSExtValue()));
    }
    if (IT->getBitWidth() <= 64U) {
      return std::unique_ptr<FixedPointValue>
	(new SFixedPoint64(SPrec,
			   CIMin->getSExtValue(),
			   CIMax->getSExtValue()));
    }
    llvm_unreachable("Types wider than 64 bits not supported.");
  }
}

std::unique_ptr<FixedPointValue>
FixedPointValue::createFromMetadata(const Instruction &I) {
  const MDNode *MDRange = I.getMetadata("errorprop.range");
  if (MDRange == nullptr)
    return nullptr;

  const IntegerType *IType =
    dyn_cast_or_null<IntegerType>(I.getType());

  return createFromMDNode(IType, *MDRange);
}

ConstantAsMetadata *
FixedPointValue::getPointPosMetadata(LLVMContext &C, bool Signed) const {
  int32_t SPointPos = (Signed) ? -this->getPointPos() : this->getPointPos();
  IntegerType *PPosType = IntegerType::get(C, 32U);
  ConstantInt *PPosCInt = ConstantInt::getSigned(PPosType, SPointPos);
  return ConstantAsMetadata::get(PPosCInt);
}

UFixedPoint32::UFixedPoint32(const unsigned PointPos)
  : FixedPointValue(PointPos), Min(0U), Max(0U) {
  assert(PointPos <= 32U && "Fractional bits cannot be more that total width.");
}

UFixedPoint32::UFixedPoint32(const unsigned PointPos,
			     const uint32_t Min, const uint32_t Max)
  : FixedPointValue(PointPos),
    Min(Min), Max(Max) {
  assert(PointPos <= 32U && "Fractional bits cannot be more that total width.");
  assert(Min <= Max && "Inconsistent bounds.");
}

FPInterval UFixedPoint32::getInterval() const {
  inter_t Exp = std::ldexp(static_cast<inter_t>(1.0),
			   -this->getPointPos());
  return FPInterval(Interval<inter_t>(static_cast<inter_t>(Min) * Exp,
				      static_cast<inter_t>(Max) * Exp));
}

MDNode *UFixedPoint32::toMetadata(LLVMContext &C) const {
  Metadata *MDs[] = {this->getPointPosMetadata(C),
		     getBoundMetadata(C, this->Min),
		     getBoundMetadata(C, this->Max)};

  return MDNode::get(C, MDs);
}

UFixedPoint64::UFixedPoint64(const unsigned PointPos)
  : FixedPointValue(PointPos), Min(0U), Max(0U) {
  assert(PointPos <= 64U && "Fractional bits cannot be more that total width.");
}

UFixedPoint64::UFixedPoint64(const unsigned PointPos,
			     const uint64_t Min, const uint64_t Max)
  : FixedPointValue(PointPos),
    Min(Min), Max(Max) {
  assert(PointPos <= 64U && "Fractional bits cannot be more that total width.");
  assert(Min <= Max && "Inconsistent bounds.");
}

FPInterval UFixedPoint64::getInterval() const {
  inter_t Exp = std::ldexp(static_cast<inter_t>(1.0),
			   -this->getPointPos());
  return FPInterval(Interval<inter_t>(static_cast<inter_t>(Min) * Exp,
				      static_cast<inter_t>(Max) * Exp));
}

MDNode *UFixedPoint64::toMetadata(LLVMContext &C) const {
  Metadata *MDs[] = {this->getPointPosMetadata(C),
		     getBoundMetadata(C, this->Min),
		     getBoundMetadata(C, this->Max)};

  return MDNode::get(C, MDs);
}

SFixedPoint32::SFixedPoint32(const unsigned PointPos)
  : FixedPointValue(PointPos), Min(0), Max(0) {
  assert(PointPos <= 32U && "Fractional bits cannot be more that total width.");
}

SFixedPoint32::SFixedPoint32(const unsigned PointPos,
			     const int32_t Min, const int32_t Max)
  : FixedPointValue(PointPos), Min(Min), Max(Max) {
  assert(PointPos <= 32U && "Fractional bits cannot be more that total width.");
  assert(Min <= Max && "Inconsistent bounds.");
}

FPInterval SFixedPoint32::getInterval() const {
  inter_t Exp = std::ldexp(static_cast<inter_t>(1.0),
			   -this->getPointPos());
  return FPInterval(Interval<inter_t>(static_cast<inter_t>(Min) * Exp,
				      static_cast<inter_t>(Max) * Exp));
}

MDNode *SFixedPoint32::toMetadata(LLVMContext &C) const {
  Metadata *MDs[] = {this->getPointPosMetadata(C, true),
		     getBoundMetadata(C, this->Min),
		     getBoundMetadata(C, this->Max)};

  return MDNode::get(C, MDs);
}

SFixedPoint64::SFixedPoint64(const unsigned PointPos)
  : FixedPointValue(PointPos), Min(0), Max(0) {
  assert(PointPos <= 64U && "Fractional bits cannot be more that total width.");
}

SFixedPoint64::SFixedPoint64(const unsigned PointPos,
			     const int64_t Min, const int64_t Max)
  : FixedPointValue(PointPos), Min(Min), Max(Max) {
  assert(PointPos <= 64U && "Fractional bits cannot be more that total width.");
  assert(Min <= Max && "Inconsistent bounds.");
}

FPInterval SFixedPoint64::getInterval() const {
  inter_t Exp = std::ldexp(static_cast<inter_t>(1.0),
			   -this->getPointPos());
  return FPInterval(Interval<inter_t>(static_cast<inter_t>(Min) * Exp,
				      static_cast<inter_t>(Max) * Exp));
}

MDNode *SFixedPoint64::toMetadata(LLVMContext &C) const {
  Metadata *MDs[] = {this->getPointPosMetadata(C, true),
		     getBoundMetadata(C, this->Min),
		     getBoundMetadata(C, this->Max)};

  return MDNode::get(C, MDs);
}

} // end namespace ErrorProp
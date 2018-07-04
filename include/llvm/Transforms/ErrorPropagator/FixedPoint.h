//===-- FixedPoint.h - Representation of fixed-point values -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declarations of
/// fixed point arithmetic type wrappers.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_FIXED_POINT_H
#define ERRORPROPAGATOR_FIXED_POINT_H

#include <cstdint>

#include "llvm/IR/Instruction.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Transforms/ErrorPropagator/AffineForms.h"

namespace ErrorProp {

/// Intermediate type for error computations.
typedef long double inter_t;

/// Interval of former fixed point values
/// An interval representing a fixed point range in the intermediate type.
class FPInterval : public Interval<inter_t> {
public:

  FPInterval()
    : PointPos(0U) {}

  FPInterval(unsigned PointPos, bool isSigned = false)
    : PointPos((isSigned) ? -PointPos : PointPos) {}

  FPInterval(unsigned PointPos, const Interval<inter_t> &I, bool isSigned = false)
    : Interval<inter_t>(I), PointPos((isSigned) ? -PointPos : PointPos) {}

  unsigned getPointPos() const {
    return std::abs(PointPos);
  }

  int getSPointPos() const {
    return PointPos;
  }

  bool isSigned() const {
    return PointPos < 0;
  }

  inter_t getRoundingError() const;

protected:
  int PointPos; ///< Number of fractional binary digits, negative if signed.
};

/// Fixed Point value type wrapper.
/// Represents the type of a fixed point value
/// as the total number of bits of the implementation (precision)
/// and the number of fractional bits (PointPos).
class FixedPointValue {
public:

  virtual unsigned getPrecision() const = 0;

  virtual unsigned getPointPos() const {
    return PointPos;
  }

  virtual bool isSigned() const = 0;

  virtual FPInterval getInterval() const = 0;

  virtual llvm::MDNode *toMetadata(llvm::LLVMContext &) const = 0;

  static std::unique_ptr<FixedPointValue>
  createFromConstantInt(int SPrec,
			const llvm::IntegerType *IT,
			const llvm::ConstantInt *CIMin,
			const llvm::ConstantInt *CIMax);

  static std::unique_ptr<FixedPointValue>
  createFromMDNode(const llvm::IntegerType *IT, const llvm::MDNode &N);

  static std::unique_ptr<FixedPointValue>
  createFromMetadata(const llvm::Instruction &);

protected:
  const unsigned PointPos; ///< Number of fractional binary digits.

  FixedPointValue(const unsigned PointPos)
    : PointPos(PointPos) {}

  llvm::ConstantAsMetadata *
  getPointPosMetadata(llvm::LLVMContext &C, bool Signed = false) const;
};

/// This class represents an interval of 32 bit unsigned fixed point values.
class UFixedPoint32 : public FixedPointValue {
public:
  UFixedPoint32(const unsigned PointPos);

  UFixedPoint32(const unsigned PointPos,
		const uint32_t Min, const uint32_t Max);

  unsigned getPrecision() const override {
    return 32U;
  }

  bool isSigned() const override {
    return false;
  }

  FPInterval getInterval() const override;

  llvm::MDNode *toMetadata(llvm::LLVMContext &) const override;

protected:
  uint32_t Min;
  uint32_t Max;
};

/// This class represents an interval of 64 bit unsigned fixed point values.
class UFixedPoint64 : public FixedPointValue {
public:
  UFixedPoint64(const unsigned PointPos);

  UFixedPoint64(const unsigned PointPos,
		const uint64_t Min, const uint64_t Max);

  unsigned getPrecision() const override {
    return 64U;
  }

  bool isSigned() const override {
    return false;
  }

  FPInterval getInterval() const override;

  llvm::MDNode *toMetadata(llvm::LLVMContext &) const override;

protected:
  uint64_t Min;
  uint64_t Max;
};

class SFixedPoint32 : public FixedPointValue {
public:
  SFixedPoint32(const unsigned PointPos);

  SFixedPoint32(const unsigned PointPos,
		const int32_t Min, const int32_t Max);

  unsigned getPrecision() const override {
    return 32U;
  }

  bool isSigned() const override {
    return true;
  }

  FPInterval getInterval() const override;

  llvm::MDNode *toMetadata(llvm::LLVMContext &) const override;

protected:
  int32_t Min;
  int32_t Max;
};

class SFixedPoint64 : public FixedPointValue {
public:
  SFixedPoint64(const unsigned PointPos);

  SFixedPoint64(const unsigned PointPos,
		const int64_t Min, const int64_t Max);

  unsigned getPrecision() const override {
    return 64U;
  }

  bool isSigned() const override {
    return true;
  }

  FPInterval getInterval() const override;

  llvm::MDNode *toMetadata(llvm::LLVMContext &) const override;

protected:
  int64_t Min;
  int64_t Max;
};

struct CmpErrorInfo {
public:
  inter_t MaxTolerance;
  bool MayBeWrong;

  CmpErrorInfo(inter_t MaxTolerance, bool MayBeWrong = true)
    : MaxTolerance(MaxTolerance), MayBeWrong(MayBeWrong) {}
};

} // end namespace ErrorProp

#endif // ERRORPROPAGATOR_FIXED_POINT_H

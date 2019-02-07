//===-- Propagators.h - Error Propagators for LLVM Instructions --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Declarations of functions that propagate fixed point computation errors
/// for each LLVM Instruction.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_PROPAGATORS_H
#define ERRORPROPAGATOR_PROPAGATORS_H

#include "llvm/IR/Instruction.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h" // togliere
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "RangeErrorMap.h"

namespace ErrorProp {

class InstructionPropagator {
public:
  InstructionPropagator(RangeErrorMap &RMap, MemorySSA &MemSSA)
    : RMap(RMap), MemSSA(MemSSA) {}

  /// Propagate errors for a Binary Operator instruction.
  bool propagateBinaryOp(Instruction &);

  /// Propagate errors for a store instruction
  /// by associating the errors of the source to the destination.
  bool propagateStore(Instruction &);

  /// Propagate the errors for a Load instruction
  /// by associating the errors of the source to it.
  bool propagateLoad(Instruction &);

  /// Propagate the errors for an Extend instruction
  /// by associating the errors of the source to it.
  bool propagateExt(Instruction &);

  /// Propagate the errors for a Trunc instruction
  /// by associating the errors of the source to it.
  bool propagateTrunc(Instruction &);

  /// Propagate errors for a SIToFP or UIToFP instruction
  /// by associating the errors of the source to it.
  bool propagateIToFP(Instruction &);

  /// Propagate errors for a FPToSI or FPToUI instruction
  /// by associating to it the error of the source plus the rounding error.
  bool propagateFPToI(Instruction &);

  /// Propagate errors for a Select instruction
  /// by associating the maximum error from the source values to it.
  bool propagateSelect(Instruction &);

  /// Propagate errors for a PHI Node
  /// by associating the maximum error from the source values to it.
  bool propagatePhi(Instruction &);

  /// Check whether the error on the operands could make this comparison wrong.
  bool checkCmp(CmpErrorMap &, Instruction &);

  /// Associate the error previously computed for the returned value
  /// to the containing function, only if larger
  /// than the one already associated (if any).
  bool propagateRet(Instruction &I);

  static bool isSpecialFunction(Function &F);

  /// Associate the error of the called function to I.
  /// Works woth both CallInst and InvokeInst.
  bool propagateCall(Instruction &I);

  /// Associate the error of the source pointer to I.
  bool propagateGetElementPtr(Instruction &I);

private:
  RangeErrorMap &RMap;
  MemorySSA &MemSSA;

  const RangeErrorMap::RangeError *getConstantFPRangeError(ConstantFP *VFP);

  const RangeErrorMap::RangeError *
  getConstantRangeError(Instruction &I, ConstantInt *VInt,
			bool DoublePP = false, const FPType *FallbackTy = nullptr);

  const RangeErrorMap::RangeError*
  getOperandRangeError(Instruction &I, Value *V,
		       bool DoublePP = false, const FPType *FallbackTy = nullptr);

  const RangeErrorMap::RangeError*
  getOperandRangeError(Instruction &I, unsigned Op,
		       bool DoublePP = false, const FPType *FallbackTy = nullptr);

  void updateArgumentRE(Value *Pointer, const RangeErrorMap::RangeError *NewRE);

  bool unOpErrorPassThrough(Instruction &I);

  static bool isSqrt(Function &F);
  static bool isLog(Function &F);
  static bool isExp(Function &F);
  static bool isAcos(Function &F);
  static bool isAsin(Function &F);
  bool propagateSqrt(Instruction &I);
  bool propagateLog(Instruction &I);
  bool propagateExp(Instruction &I);
  bool propagateAcos(Instruction &I);
  bool propagateAsin(Instruction &I);
  bool propagateSpecialCall(Instruction &I, Function &Called);

  inter_t computeMinRangeDiff(const FPInterval &R1, const FPInterval &R2);
};

} // end of namespace ErrorProp

#endif

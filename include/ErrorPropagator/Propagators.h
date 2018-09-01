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
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "RangeErrorMap.h"

namespace ErrorProp {

/// Propagate errors for a Binary Operator instruction.
bool propagateBinaryOp(RangeErrorMap &, Instruction &);

/// Propagate errors for a store instruction
/// by associating the errors of the source to the destination.
bool propagateStore(RangeErrorMap &, MemorySSA &, Instruction &);

/// Propagate the errors for a Load instruction
/// by associating the errors of the source to it.
bool propagateLoad(RangeErrorMap &, MemorySSA &, Instruction &);

/// Propagate the errors for an Extend instruction
/// by associating the errors of the source to it.
bool propagateExt(RangeErrorMap &, Instruction &);

/// Propagate the errors for a Trunc instruction
/// by associating the errors of the source to it.
bool propagateTrunc(RangeErrorMap &, Instruction &);

/// Propagate errors for a SIToFP or UIToFP instruction
/// by associating the errors of the source to it.
bool propagateIToFP(RangeErrorMap &, Instruction &);

/// Propagate errors for a FPToSI or FPToUI instruction
/// by associating to it the error of the source plus the rounding error.
bool propagateFPToI(RangeErrorMap &, Instruction &);

/// Propagate errors for a Select instruction
/// by associating the maximum error from the source values to it.
bool propagateSelect(RangeErrorMap &, Instruction &);

/// Propagate errors for a PHI Node
/// by associating the maximum error from the source values to it.
bool propagatePhi(RangeErrorMap &, Instruction &);

/// Check whether the error on the operands could make this comparison wrong.
bool checkCmp(RangeErrorMap &, CmpErrorMap &, Instruction &);

/// Associate the error previously computed for the returned value
/// to the containing function, only if larger
/// than the one already associated (if any).
bool propagateRet(RangeErrorMap &RMap, Instruction &I);

bool isSpecialFunction(Function &F);

/// Associate the error of the called function to I.
/// Works woth both CallInst and InvokeInst.
bool propagateCall(RangeErrorMap &RMap, Instruction &I);

/// Associate the error of the source pointer to I.
bool propagateGetElementPtr(RangeErrorMap &RMap, Instruction &I);

#define DEFAULT_RE_COUNT 8U

void findMemSSAError(RangeErrorMap &RMap, MemorySSA &MemSSA,
		     Instruction *I, MemoryAccess *MA,
		     SmallSet<MemoryAccess *, DEFAULT_RE_COUNT> &Visited,
		     SmallVectorImpl<const RangeErrorMap::RangeError *> &Res);

} // end of namespace ErrorProp

#endif

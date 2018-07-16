//===-- FunctionErrorPropagator.h - Error Propagator ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Error propagator for fixed point computations in a single function.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_FUNCTIONERRORPROPAGATOR_H
#define ERRORPROPAGATOR_FUNCTIONERRORPROPAGATOR_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemorySSA.h"

#include "RangeErrorMap.h"
#include "FunctionCopyMap.h"

namespace ErrorProp {

using namespace llvm;

/// Propagates errors of fixed point computations in a single function.
class FunctionErrorPropagator {
public:
  FunctionErrorPropagator(Pass &EPPass,
			  Function &F,
			  FunctionCopyManager &FCMap)
    : EPPass(EPPass), F(F), FCMap(FCMap),
      FCopy(FCMap.getFunctionCopy(&F)), RMap(),
      CmpMap(CMPERRORMAP_NUMINITBUCKETS), MemSSA(nullptr) {}

  /// Propagate errors, cloning the function if code modifications are required.
  /// GlobRMap maps global variables and functions to their errors,
  /// and the error computed for this function's return value is stored in it;
  /// Args contains pointers to the actual parameters of a call to this function;
  /// if GenMetadata is true, computed errors are attached
  /// to each instruction as metadata.
  void computeErrorsWithCopy(RangeErrorMap &GlobRMap,
			     SmallVectorImpl<Value *> *Args = nullptr,
			     bool GenMetadata = false);

protected:
  /// Compute errors instruction by instruction.
  void computeFunctionErrors(SmallVectorImpl<Value *> *ArgErrs);

  /// Compute errors for a single instruction,
  /// using the range from metadata attached to it.
  void computeInstructionErrors(Instruction &I);

  /// Compute errors for a single instruction.
  void dispatchInstruction(Instruction &I);

  /// Compute the error on the return value of another function.
  void prepareErrorsForCall(Instruction &I);

  /// Attach error metadata to the original function.
  void attachErrorMetadata();

  Pass &EPPass;
  Function &F;
  FunctionCopyManager &FCMap;

  Function *FCopy;
  RangeErrorMap RMap;
  CmpErrorMap CmpMap;
  MemorySSA *MemSSA;
};

} // end namespace ErrorProp

#endif

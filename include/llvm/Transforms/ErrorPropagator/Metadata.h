//===-- Metadata.h - Metadata Utils for ErrorPropagator ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Declarations of utility functions that handle metadata in Error Propagator.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_METADATA_H
#define ERRORPROPAGATOR_METADATA_H

#include <utility>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/Transforms/ErrorPropagator/AffineForms.h"
#include "llvm/Transforms/ErrorPropagator/FixedPoint.h"

#define COMP_ERROR_METADATA    "errorprop.abserror"
#define FUNCTION_ARGS_METADATA "errorprop.argsrange"
#define WRONG_CMP_METADATA     "errorprop.wrongcmptol"
#define GLOBAL_VAR_METADATA    "errorprop.globalre"

namespace ErrorProp {

/// Attach metadata containing the computed error to the given instruction.
void setErrorMetadata(llvm::Instruction &, const AffineForm<inter_t> &);

/// Attach metadata containing ranges and initial absolute errors
/// for each argument of the given function.
void setFunctionArgsMetadata(llvm::Function &,
			     const llvm::ArrayRef<std::pair<
			     const FixedPointValue *,
			     const AffineForm<inter_t> *> >);

/// Extract function argument ranges and initial errors
/// from Function metadata.
llvm::SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const llvm::Function &);

/// Check whether error propagation must be performed on this function.
bool propagateFunction(const llvm::Function &);

/// Attach maximum error tolerance to Cmp instruction.
/// The metadata are attached only if the comparison may be wrong.
void setCmpErrorMetadata(llvm::Instruction &, const CmpErrorInfo &);

/// Attach Range and Error metadata to global object V.
void setGlobalVariableMetadata(llvm::GlobalObject &V,
			       const FixedPointValue *Range,
			       const AffineForm<inter_t> *Error);

/// Check whether V has global variable metadata attached to it.
bool hasGlobalVariableMetadata(const llvm::GlobalObject &V);

/// Retrieve Range and Error from metadata attached to global object V.
std::pair<FPInterval, AffineForm<inter_t> >
retrieveGlobalVariableRangeError(const llvm::GlobalObject &V);

}

#endif

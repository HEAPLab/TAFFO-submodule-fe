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
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/ADT/Optional.h"
#include "ErrorPropagator/EPUtils/AffineForms.h"
#include "ErrorPropagator/EPUtils/FixedPoint.h"

#define INPUT_INFO_METADATA    "taffo.info"
#define RANGE_METADATA         "taffo.info"
#define GLOBAL_VAR_METADATA    "taffo.info"
#define FUNCTION_ARGS_METADATA "taffo.funinfo"
#define COMP_ERROR_METADATA    "taffo.abserror"
#define WRONG_CMP_METADATA     "taffo.wrongcmptol"
#define MAX_REC_METADATA       "taffo.maxrec"
#define UNROLL_COUNT_METADATA  "taffo.unroll"

namespace ErrorProp {

/// Structure containing pointers to Type, Range, and initial Error
/// of an LLVM Value.
/// See FixedPoint.h for docs about TType.
/// Range may be created with Interval<inter_t>(Min, Max).
struct InputInfo {
  TType *Ty;
  Interval<inter_t> *Range;
  inter_t *Error;

  InputInfo()
    : Ty(nullptr), Range(nullptr), Error(nullptr) {}

  InputInfo(TType *T, Interval<inter_t> *Range, inter_t *Error)
    : Ty(T), Range(Range), Error(Error) {}
};

/// Attach to Instruction I an input info metadata node
/// containing Type info T, Range, and initial Error.
void setInputInfoMetadata(Instruction &I, const InputInfo &IInfo);

/// Extract range information from Instruction metadata.
llvm::Optional<FPInterval> retrieveRangeFromMetadata(llvm::Instruction &I);

/// Attach metadata containing the computed error to the given instruction.
/// E is an affine form containing the error terms computed for Instruction I.
void setErrorMetadata(llvm::Instruction &I, const AffineForm<inter_t> &E);

/// Attach metadata containing types, ranges and initial absolute errors
/// for each argument of the given function.
/// AInfo is an array/vector of InputInfo object containing type,
/// range and initial error of each formal parameter of F.
/// Each InputInfo object refers to the function parameter with the same index.
void setFunctionArgsMetadata(llvm::Function &F,
			     const ArrayRef<InputInfo> AInfo);

/// Extract function argument ranges and initial errors
/// from Function metadata.
llvm::SmallVector<std::pair<FPInterval, AffineForm<inter_t> >, 1U>
retrieveArgsRangeError(const llvm::Function &);

/// Attach maximum error tolerance to Cmp instruction.
/// The metadata are attached only if the comparison may be wrong.
void setCmpErrorMetadata(llvm::Instruction &, const CmpErrorInfo &);

/// Attach Input Info metadata to global object V.
/// IInfo contains pointers to type, range and initial error
/// to be attached to global object V as metadata.
void setGlobalVariableMetadata(GlobalObject &V, const InputInfo &IInfo);

/// Check whether V has global variable metadata attached to it.
bool hasGlobalVariableMetadata(const llvm::GlobalObject &V);

/// Retrieve Range and Error from metadata attached to global object V.
std::pair<FPInterval, AffineForm<inter_t> >
retrieveGlobalVariableRangeError(const llvm::GlobalObject &V);

/// Attach MaxRecursionCount to the given function.
/// Attaches metadata containing the maximum number of recursive calls
/// that is allowed for function F.
void
setMaxRecursionCountMetadata(llvm::Function &F, unsigned MaxRecursionCount);

/// Read the MaxRecursionCount from metadata attached to function F.
/// Returns 0 if no metadata have been found.
unsigned
retrieveMaxRecursionCount(const llvm::Function &F);

/// Attach unroll count metadata to loop L.
/// Attaches UnrollCount as the number of times to unroll loop L
/// as metadata to the terminator instruction of the loop header.
void
setLoopUnrollCountMetadata(llvm::Loop &L, unsigned UnrollCount);

/// Read loop unroll count from metadata attached to the header of L.
llvm::Optional<unsigned> retrieveLoopUnrollCount(const llvm::Loop &L);

}

#endif

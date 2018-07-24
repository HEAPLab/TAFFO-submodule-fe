//===-- RangeErrorMap.h - Range and Error Maps ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains classes that map Instructions and other Values
/// to the corresponding computed ranges and errors.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_RANGEERRORMAP_H
#define ERRORPROPAGATOR_RANGEERRORMAP_H

#include <map>
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/DenseMap.h"
#include "EPUtils/FixedPoint.h"
#include "EPUtils/AffineForms.h"

namespace ErrorProp {

using namespace llvm;

class RangeErrorMap {
public:
  typedef std::pair<FPInterval, AffineForm<inter_t> > RangeError;

  RangeErrorMap() : REMap() {}

  const FPInterval *getRange(const Value *) const;

  const AffineForm<inter_t> *getError(const Value *) const;

  const RangeError*
  getRangeError(const Value *) const;

  /// Set error for Value V.
  /// Err cannot be a reference to an error contained in this map.
  void setError(const Value *V, const AffineForm<inter_t> &Err);

  /// Set range and error for Value V.
  /// RE cannot be a reference to a RangeError contained in this map.
  void setRangeError(const Value *V,
		     const RangeError &RE);

  void erase(const Value *V) {
    REMap.erase(V);
  }

  /// Set all errors to 0 (but keep range untouched).
  void resetErrors();

  /// Retrieve range for instruction I from metadata.
  void retrieveRange(Instruction *I);

  /// Retrieve ranges and errors for arguments of function F from metadata.
  void retrieveRangeErrors(const Function &F);

  /// Associate the errors of the actual parameters of F contained in Args
  /// to the corresponding formal parameters.
  void applyArgumentErrors(Function &F,
			   SmallVectorImpl<Value *> *Args);

  /// Retrieve range and error for global variable V, and add it to the map.
  void retrieveRangeError(const GlobalObject &V);

protected:
  std::map<const Value *, RangeError> REMap;

}; // end class RangeErrorMap

typedef DenseMap<Value *, CmpErrorInfo> CmpErrorMap;
#define CMPERRORMAP_NUMINITBUCKETS 4U

} // end namespace ErrorProp

#endif

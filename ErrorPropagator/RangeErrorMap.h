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
#include <memory>
#include "llvm/Support/Casting.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Optional.h"
#include "Metadata.h"
#include "FixedPoint.h"
#include "AffineForms.h"
#include "StructErrorMap.h"

namespace ErrorProp {

using namespace llvm;

class TargetErrors {
public:
  void updateTarget(const Value *V, const inter_t &Error);
  void updateTarget(const Instruction *I, const inter_t &Error);
  void updateTarget(const GlobalVariable *V, const inter_t &Error);
  void updateTarget(StringRef T, const inter_t &Error);
  void updateAllTargets(const TargetErrors &Other);

  inter_t getErrorForTarget(StringRef T) const;

  void printTargetErrors(raw_ostream &OS) const;

protected:
  DenseMap<StringRef, inter_t> Targets;
};

class RangeErrorMap {
public:
  typedef std::pair<FPInterval, Optional<AffineForm<inter_t> > > RangeError;

  RangeErrorMap(MetadataManager &MDManager)
    : REMap(), MDMgr(&MDManager) {}

  const FPInterval *getRange(const Value *) const;

  const AffineForm<inter_t> *getError(const Value *) const;

  const RangeError*
  getRangeError(const Value *) const;

  /// Set error for Value V.
  /// Err cannot be a reference to an error contained in this map.
  void setError(const Value *V, const AffineForm<inter_t> &Err);

  /// Set range and error for Value V.
  /// RE cannot be a reference to a RangeError contained in this map.
  void setRangeError(const Value *V, const RangeError &RE);

  void erase(const Value *V) {
    REMap.erase(V);
  }

  /// Retrieve range for instruction I from metadata.
  /// Return true if initial error metadata was found attached to I.
  bool retrieveRangeError(Instruction &I);

  /// Retrieve ranges and errors for arguments of function F from metadata.
  void retrieveRangeErrors(const Function &F);

  /// Associate the errors of the actual parameters of F contained in Args
  /// to the corresponding formal parameters.
  void applyArgumentErrors(Function &F,
			   SmallVectorImpl<Value *> *Args);

  /// Retrieve range and error for global variable V, and add it to the map.
  void retrieveRangeError(const GlobalObject &V);

  MetadataManager &getMetadataManager() { return *MDMgr; }

  const RangeError *getStructRangeError(Value *Pointer) const;
  void setStructRangeError(Value *Pointer, const RangeError &RE);

  void initArgumentBindings(Function &F, const ArrayRef<Value *> AArgs) {
    SEMap.initArgumentBindings(F, AArgs);
  }
  void updateStructErrors(const RangeErrorMap &O, const ArrayRef<Value *> Pointers) {
    SEMap.updateStructTree(O.SEMap, Pointers);
  }

  void updateTargets(const RangeErrorMap &Other);
  void printTargetErrors(raw_ostream &OS) const { TErrs.printTargetErrors(OS); }

protected:
  std::map<const Value *, RangeError> REMap;
  MetadataManager *MDMgr;
  StructErrorMap SEMap;
  TargetErrors TErrs;
}; // end class RangeErrorMap

typedef DenseMap<Value *, CmpErrorInfo> CmpErrorMap;
#define CMPERRORMAP_NUMINITBUCKETS 4U


} // end namespace ErrorProp

#endif
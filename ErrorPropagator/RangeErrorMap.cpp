//===-- RangeErrorMap.cpp - Range and Error Maps ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains definitons of members of classes
/// that map Instructions/Arguments to the corresponding
/// computed ranges and errors.
///
//===----------------------------------------------------------------------===//

#include "RangeErrorMap.h"

#include <utility>
#include "llvm/Support/Debug.h"

namespace ErrorProp {

#define DEBUG_TYPE "errorprop"

const FPInterval *RangeErrorMap::getRange(const Value *I) const {
  auto RError = REMap.find(I);
  if (RError == REMap.end()) {
    return nullptr;
  }
  return &((RError->second).first);
}

const AffineForm<inter_t> *RangeErrorMap::getError(const Value *I) const {
  auto RError = REMap.find(I);
  if (RError == REMap.end()) {
    return nullptr;
  }
  const Optional<AffineForm<inter_t> > &Error = RError->second.second;
  if (Error.hasValue())
    return Error.getPointer();
  else
    return nullptr;
}

const RangeErrorMap::RangeError*
RangeErrorMap::getRangeError(const Value *I) const {
  auto RE = REMap.find(I);
  if (RE == REMap.end()) {
    return nullptr;
  }
  return &(RE->second);
}

void RangeErrorMap::setError(const Value *I, const AffineForm<inter_t> &E) {
  // If Range does not exist, the default is created.
  REMap[I].second = E;

  TErrs.updateTarget(I, E.noiseTermsAbsSum());
}

void RangeErrorMap::setRangeError(const Value *I,
				  const RangeError &RE) {
  REMap[I] = RE;

  if (RE.second.hasValue())
    TErrs.updateTarget(I, RE.second.getValue().noiseTermsAbsSum());
}

bool RangeErrorMap::retrieveRangeError(Instruction &I) {
  InputInfo *II = MDMgr->retrieveInputInfo(I);
  if (II == nullptr)
    return false;

  if (II->IError == nullptr) {
    REMap[&I] = std::make_pair(FPInterval(II), NoneType());
    return false;
  }
  else {
    REMap[&I] = std::make_pair(FPInterval(II), AffineForm<inter_t>(0.0, *II->IError));
    return true;
  }
}

void RangeErrorMap::retrieveRangeErrors(const Function &F) {
  SmallVector<MDInfo *, 1U> REs;
  MDMgr->retrieveArgumentInputInfo(F, REs);

  auto REIt = REs.begin(), REEnd = REs.end();
  for (Function::const_arg_iterator Arg = F.arg_begin(), ArgE = F.arg_end();
       Arg != ArgE && REIt != REEnd; ++Arg, ++REIt) {
    // TODO: struct support
    InputInfo *ii = dyn_cast<InputInfo>(*REIt);
    if (ii == nullptr)
      continue;
    if (ii->IRange == nullptr)
      continue;

    FPInterval FPI(ii);

    DEBUG(dbgs() << "Retrieving data for Argument " << Arg->getName() << "... "
	  << "Range: [" << static_cast<double>(FPI.Min) << ", "
	  << static_cast<double>(FPI.Max) << "], Error: ");

    if (FPI.hasInitialError()) {
      AffineForm<inter_t> Err(0.0, FPI.getInitialError());
      this->setRangeError(Arg, std::make_pair(FPI, Err));

      DEBUG(dbgs() << FPI.getInitialError() << ".\n");
    }
    else {
      this->setRangeError(Arg, std::make_pair(FPI, NoneType()));

      DEBUG(dbgs() << "none.\n");
    }
  }
}

void RangeErrorMap::applyArgumentErrors(Function &F,
					SmallVectorImpl<Value *> *Args) {
  if (Args == nullptr)
    return;

  auto FArg = F.arg_begin();
  auto FArgEnd = F.arg_end();
  for (auto AArg = Args->begin(), AArgEnd = Args->end();
       AArg != AArgEnd && FArg != FArgEnd;
       ++AArg, ++FArg) {
    Value *AArgV = *AArg;
    const AffineForm<inter_t> *Err = this->getError(AArgV);
    if (Err == nullptr) {
      DEBUG(
	    dbgs() << "No pre-computed error available for formal parameter (" << *FArg << ")";
	    if (AArgV != nullptr)
	      dbgs() << "from actual parameter (" << *AArgV << ").\n";
	    else
	      dbgs() << ".\n";
	    );
      continue;
    }

    this->setError(&*FArg, *Err);

    DEBUG(dbgs() << "Pre-computed error applied to formal parameter (" << *FArg
	  << ") from actual parameter (" << *AArgV
	  << "): " << static_cast<double>(Err->noiseTermsAbsSum()) << ".\n");
  }
}

void RangeErrorMap::retrieveRangeError(const GlobalObject &V) {
  DEBUG(dbgs() << "Retrieving data for Global Variable " << V.getName() << "... ");

  InputInfo *II = MDMgr->retrieveInputInfo(V);
  if (II == nullptr) {
    DEBUG(dbgs() << "ignored (no data).\n");
    return;
  }

  FPInterval FPI(II);

  DEBUG(dbgs() << "Range: [" << static_cast<double>(FPI.Min) << ", "
	<< static_cast<double>(FPI.Max) << "], Error: ");

  if (FPI.hasInitialError()) {
    REMap[&V] = std::make_pair(FPI, AffineForm<inter_t>(0.0, FPI.getInitialError()));

    DEBUG(dbgs() << FPI.getInitialError() << ".\n");
  }
  else {
    REMap[&V] = std::make_pair(FPI, NoneType());

    DEBUG(dbgs() << "none.\n");
  }
}

const RangeErrorMap::RangeError *
RangeErrorMap::getStructRangeError(Value *Pointer) const {
  assert(isa<PointerType>(Pointer->getType()));

  return SEMap.getFieldError(Pointer);
}

void RangeErrorMap::setStructRangeError(Value *Pointer, const RangeError &RE) {
  assert(isa<PointerType>(Pointer->getType()));

  SEMap.setFieldError(Pointer, RE);
}

void RangeErrorMap::updateTargets(const RangeErrorMap &Other) {
  this->TErrs.updateAllTargets(Other.TErrs);
}

void TargetErrors::updateTarget(const Value *V, const inter_t &Error) {
  if (isa<Instruction>(V))
    updateTarget(cast<Instruction>(V), Error);
  else if (isa<GlobalVariable>(V))
    updateTarget(cast<GlobalVariable>(V), Error);
}

void TargetErrors::updateTarget(const Instruction *I, const inter_t &Error) {
  assert(I != nullptr);
  Optional<StringRef> Target = MetadataManager::retrieveTargetMetadata(*I);
  if (Target.hasValue())
    updateTarget(Target.getValue(), Error);
}

void TargetErrors::updateTarget(const GlobalVariable *V, const inter_t &Error) {
  assert(V != nullptr);
  Optional<StringRef> Target = MetadataManager::retrieveTargetMetadata(*V);
  if (Target.hasValue())
    updateTarget(Target.getValue(), Error);
}

void TargetErrors::updateTarget(StringRef T, const inter_t &Error) {
  Targets[T] = std::max(Targets[T], Error);
}

void TargetErrors::updateAllTargets(const TargetErrors &Other) {
  for (auto &T : Other.Targets)
    this->updateTarget(T.first, T.second);
}

inter_t TargetErrors::getErrorForTarget(StringRef T) const {
  auto Error = Targets.find(T);
  if (Error == Targets.end())
    return 0;

  return Error->second;
}

void TargetErrors::printTargetErrors(raw_ostream &OS) const {
  for (auto &T : Targets) {
    OS << "Computed error for target " << T.first << ": "
       << static_cast<double>(T.second) << "\n";
  }
}

} // end namespace ErrorProp

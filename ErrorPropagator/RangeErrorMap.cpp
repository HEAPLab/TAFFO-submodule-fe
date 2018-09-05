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

#include "ErrorPropagator/RangeErrorMap.h"

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
  SmallVector<InputInfo *, 1U> REs;
  MDMgr->retrieveArgumentInputInfo(F, REs);

  auto REIt = REs.begin(), REEnd = REs.end();
  for (Function::const_arg_iterator Arg = F.arg_begin(), ArgE = F.arg_end();
       Arg != ArgE && REIt != REEnd; ++Arg, ++REIt) {
    FPInterval FPI(*REIt);

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
    const AffineForm<inter_t> *Err = this->getError(*AArg);
    if (Err == nullptr)
      continue;

    this->setError(&*FArg, *Err);

    DEBUG(dbgs() << "Pre-computed error applied to argument " << FArg->getName()
	  << ": " << static_cast<double>(Err->noiseTermsAbsSum()) << ".\n");
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

void RangeErrorMap::updateStructElemError(StructType *ST, unsigned Idx,
					  const inter_t &Error) {
  auto StrIt = StructMap.find(ST);
  if (StrIt == StructMap.end()) {
    StructMap[ST].append(ST->getNumElements(), NoneType());
  }
  Optional<inter_t> &OldErr = StructMap[ST][Idx];
  if (!OldErr.hasValue() || OldErr < Error)
    StructMap[ST][Idx] = Error;

  DEBUG(dbgs() << "Updated errors of Struct " << ST->getName() << ": ");
  DEBUG(printStructErrs(ST, dbgs()));
  DEBUG(dbgs() << "\n");
}

void RangeErrorMap::updateStructElemError(StoreInst &SI, const AffineForm<inter_t> *Error) {
  if (Error == nullptr)
    return;

  if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(SI.getPointerOperand())) {
    auto IdxI = GEPI->idx_begin();
    ++IdxI;
    Type *CurType = GEPI->getSourceElementType();
    StructType *StrType = nullptr;
    unsigned Idx = 0U;
    while (IdxI != GEPI->idx_end()) {
      if (isa<StructType>(CurType)) {
	StrType = cast<StructType>(CurType);
	Value *IdxV = *IdxI;
	if (!isa<ConstantInt>(IdxV))
	  return;
	Idx = cast<ConstantInt>(IdxV)->getZExtValue();
	CurType = StrType->getElementType(Idx);
	++IdxI;
      }
      else if (SequentialType *SeqTy = dyn_cast<SequentialType>(CurType)) {
	CurType = SeqTy->getElementType();
	++IdxI;
      }
      else
	return;
    }
    if (StrType != nullptr) {
      updateStructElemError(StrType, Idx, Error->noiseTermsAbsSum());
    }
  }
}

void RangeErrorMap::updateStructElemError(const RangeErrorMap &Other) {
  for (auto &StructErrs : Other.StructMap) {
    unsigned Idx = 0U;
    for (const Optional<inter_t> &ElemError : StructErrs.second) {
      if (ElemError.hasValue())
	updateStructElemError(StructErrs.first, Idx, ElemError.getValue());

      ++Idx;
    }
  }
}

void RangeErrorMap::printStructErrs(StructType *ST, raw_ostream &OS) const {
  auto STErrs = StructMap.find(ST);
  if (STErrs == StructMap.end()) {
    OS << "null";
    return;
  }
  OS << "{ ";
  for (const Optional<inter_t> &Err : STErrs->second) {
    if (Err.hasValue())
      OS << static_cast<double>(Err.getValue()) << ", ";
    else
      OS << "null, ";
  }
  OS << "}";
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

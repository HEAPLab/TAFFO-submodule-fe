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
  return &((RError->second).second);
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
}

void RangeErrorMap::setRangeError(const Value *I,
				  const RangeError &RE) {
  REMap[I] = RE;
}

bool RangeErrorMap::retrieveRangeError(Instruction &I) {
  InputInfo *II = MDMgr->retrieveInputInfo(I);
  if (II == nullptr)
    return false;

  if (II->IError == nullptr) {
    REMap[&I] = std::make_pair(FPInterval(II), AffineForm<inter_t>());
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
    AffineForm<inter_t> Err(0.0, FPI.getInitialError());

    DEBUG(dbgs() << "Retrieving data for Argument " << Arg->getName() << "... "
	  << "Range: [" << static_cast<double>(FPI.Min) << ", "
	  << static_cast<double>(FPI.Max) << "], Error: "
	  << FPI.getInitialError() << ".\n");

    this->setRangeError(Arg, std::make_pair(FPI, Err));
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

    AffineForm<inter_t> ErrCopy = *Err;
    this->setError(&*FArg, ErrCopy);

    DEBUG(dbgs() << "Pre-computed error applied to argument " << FArg->getName()
	  << ": " << static_cast<double>(ErrCopy.noiseTermsAbsSum()) << ".\n");
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
  REMap[&V] = std::make_pair(FPI, AffineForm<inter_t>(0.0, FPI.getInitialError()));

  DEBUG(RangeError &RE = REMap[&V];
	dbgs() << "Range: [" << static_cast<double>(RE.first.Min) << ", "
	<< static_cast<double>(RE.first.Max) << "], Error: "
	<< static_cast<double>(RE.second.noiseTermsAbsSum()) << ".\n");
}

} // end namespace ErrorProp

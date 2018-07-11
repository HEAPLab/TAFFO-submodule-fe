//===-- FunctionCopyMap.h - Function Clones Management ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains a class that creates and keeps track
/// of function clones, in which loops are unrolled.
///
//===----------------------------------------------------------------------===//

#ifndef ERRORPROPAGATOR_FUNCTIONCOPYMAP_H
#define ERRORPROPAGATOR_FUNCTIONCOPYMAP_H

#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Function.h"
#include <map>

namespace ErrorProp {

using namespace llvm;

struct FunctionCopyCount {
  Function *Copy = nullptr;
  ValueToValueMapTy VMap;
  unsigned RecCount = 0U;
  unsigned MaxRecCount = 1U;
};

void UnrollLoops(Function &F, unsigned DefaultUnrollCount,
		 TargetLibraryInfo &TLI);

class FunctionCopyManager {
public:

  FunctionCopyManager(unsigned MaxRecursionCount,
		      unsigned DefaultUnrollCount,
		      bool NoLoopUnroll,
		      TargetLibraryInfo &TLI)
    : MaxRecursionCount(MaxRecursionCount),
      NoLoopUnroll(NoLoopUnroll),
      DefaultUnrollCount(DefaultUnrollCount),
      TLI(TLI) {}

  Function *getFunctionCopy(Function *F) {
    FunctionCopyCount *FCData = prepareFunctionData(F);
    assert(FCData != nullptr);

    return FCData->Copy;
  }

  unsigned getRecursionCount(Function *F) {
    auto FCData = FCMap.find(F);
    if (FCData == FCMap.end())
      return 0U;

    return FCData->second.RecCount;
  }

  unsigned getMaxRecursionCount(Function *F) {
    auto FCData = FCMap.find(F);
    if (FCData == FCMap.end())
      return MaxRecursionCount;

    return FCData->second.MaxRecCount;
  }

  void setRecursionCount(Function *F, unsigned Count) {
    FunctionCopyCount *FCData = prepareFunctionData(F);
    assert(FCData != nullptr);

    FCData->RecCount = Count;
  }

  unsigned incRecursionCount(Function *F) {
    FunctionCopyCount *FCData = prepareFunctionData(F);
    assert(FCData != nullptr);

    unsigned Old = FCData->RecCount;
    ++FCData->RecCount;

    return Old;
  }

  bool maxRecursionCountReached(Function *F) {
    auto FCData = FCMap.find(F);
    if (FCData == FCMap.end())
      return false;

    return FCData->second.RecCount >= FCData->second.MaxRecCount;
  }

  ValueToValueMapTy *getValueToValueMap(Function *F) {
    auto FCData = FCMap.find(F);
    if (FCData == FCMap.end())
      return nullptr;

    return &FCData->second.VMap;
  }

  ~FunctionCopyManager();

protected:
  typedef std::map<Function *, FunctionCopyCount> FunctionCopyMap;

  FunctionCopyMap FCMap;

  unsigned MaxRecursionCount;
  unsigned DefaultUnrollCount;
  bool NoLoopUnroll;
  TargetLibraryInfo &TLI;

  FunctionCopyCount *prepareFunctionData(Function *F);
};

} // end namespace ErrorProp

#endif

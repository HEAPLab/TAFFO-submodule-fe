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

#include "RangeErrorMap.h"
#include "FunctionCopyMap.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallSet.h"
#include <vector>
#include "llvm/Analysis/MemorySSA.h"

namespace ErrorProp {

using namespace llvm;
using namespace mdutils;

/// Propagates errors of fixed point computations in a single function.
class FunctionErrorPropagator {
public:
  FunctionErrorPropagator(Pass &EPPass,
			  Function &F,
			  FunctionCopyManager &FCMap,
			  MetadataManager &MDManager)
    : EPPass(EPPass), F(F), FCMap(FCMap),
      FCopy(FCMap.getFunctionCopy(&F)), RMap(MDManager),
      CmpMap(CMPERRORMAP_NUMINITBUCKETS), MemSSA(nullptr),
      Cloned(true) {
    if (FCopy == nullptr) {
      FCopy = &F;
      Cloned = false;
    }
  }

  /// Propagate errors, cloning the function if code modifications are required.
  /// GlobRMap maps global variables and functions to their errors,
  /// and the error computed for this function's return value is stored in it;
  /// Args contains pointers to the actual parameters of a call to this function;
  /// if GenMetadata is true, computed errors are attached
  /// to each instruction as metadata.
  void computeErrorsWithCopy(RangeErrorMap &GlobRMap,
			     SmallVectorImpl<Value *> *Args = nullptr,
			     bool GenMetadata = false);

  RangeErrorMap &getRMap() { return RMap; }

protected:
  /// Compute errors instruction by instruction.
  void computeFunctionErrors(SmallVectorImpl<Value *> *ArgErrs);

  /// Compute errors for a single instruction,
  /// using the range from metadata attached to it.
  void computeInstructionErrors(Instruction &I);

  /// Compute errors for a single instruction.
  bool dispatchInstruction(Instruction &I);

  /// Compute the error on the return value of another function.
  void prepareErrorsForCall(Instruction &I);

  /// Transfer the errors computed locally to the actual parameters of the function call,
  /// but only if they are pointers.
  void applyActualParametersErrors(RangeErrorMap &GlobRMap,
				   SmallVectorImpl<Value *> *Args);

  /// Attach error metadata to the original function.
  void attachErrorMetadata();

  /// Returns true if I may overflow, according to range data.
  bool checkOverflow(Instruction &I);

  Pass &EPPass;
  Function &F;
  FunctionCopyManager &FCMap;

  Function *FCopy;
  RangeErrorMap RMap;
  CmpErrorMap CmpMap;
  MemorySSA *MemSSA;
  bool Cloned;
};

/// Schedules basic blocks of a function so that all BBs
/// that could be executed before another BB come before it in the ordering.
/// This is a sort of topological ordering that takes loops into account.
class BBScheduler {
public:
  typedef std::vector<BasicBlock *> queue_type;
  typedef queue_type::reverse_iterator iterator;

  BBScheduler(Function &F, LoopInfo &LI)
    : Queue(), Set(), LInfo(LI) {
    Queue.reserve(F.size());
    enqueueChildren(&F.getEntryBlock());
  }

  bool empty() const {
    return Queue.empty();
  }

  iterator begin() {
    return Queue.rbegin();
  }

  iterator end() {
    return Queue.rend();
  }

protected:
  queue_type Queue;
  SmallSet<BasicBlock *, 8U> Set;
  LoopInfo &LInfo;

  /// Put BB and all of its successors in the queue.
  void enqueueChildren(BasicBlock *BB);
  /// True if Dst is an exiting or external block wrt Loop L.
  bool isExiting(BasicBlock *Dst, Loop *L) const;
};

} // end namespace ErrorProp

#endif
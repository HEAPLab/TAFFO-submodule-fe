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
    if ((*REIt)->IRange == nullptr)
      continue;

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

StructNode::StructNode(const StructNode &SN)
  : StructTree(SN), Fields(), SType(SN.SType) {
  this->Fields.reserve(SN.Fields.size());
  for (const std::unique_ptr<StructTree> &STPtr : SN.Fields) {
    std::unique_ptr<StructTree> New;
    if (STPtr != nullptr)
      New.reset(STPtr->clone());
    this->Fields.push_back(std::move(New));
  }
}

StructNode &StructNode::operator=(const StructNode &O) {
  this->SType = O.SType;
  this->Fields.clear();
  this->Fields.reserve(O.Fields.size());
  for (const std::unique_ptr<StructTree> &STPtr : O.Fields) {
    std::unique_ptr<StructTree> New(STPtr->clone());
    this->Fields.push_back(std::move(New));
  }

  return *this;
}

Value *StructTreeWalker::retrieveRootPointer(Value *P) {
  IndexStack.clear();
  return navigatePointerTreeToRoot(P);
}

StructError *StructTreeWalker::getOrCreateFieldNode(StructTree *Root) {
  return navigateStructTree(Root, true);
}

StructError *StructTreeWalker::getFieldNode(StructTree *Root) {
  return navigateStructTree(Root, false);
}

StructTree *StructTreeWalker::makeRoot(Value *P) {
  StructType *ST = cast<StructType>(cast<PointerType>(P->getType())->getElementType());
  return new StructNode(ST);
}

Value *StructTreeWalker::navigatePointerTreeToRoot(Value *P) {
  assert(P != nullptr);
  if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(P)) {
    auto IdxIt = GEPI->idx_begin();
    ++IdxIt;
    for (; IdxIt != GEPI->idx_end(); ++IdxIt)
      IndexStack.push_back(parseIndex(*IdxIt));

    return navigatePointerTreeToRoot(GEPI->getPointerOperand());
  }
  else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(P)) {
    if (CE->isGEPWithNoNotionalOverIndexing()) {
      auto OpIt = CE->op_begin();

      // Get the Pointer Operand.
      assert(OpIt != CE->op_end());
      Value *PointerOp = *OpIt;
      assert(PointerOp->getType()->isPointerTy());
      ++OpIt;

      // Discard first index.
      assert(OpIt != CE->op_end());
      ++OpIt;

      // Push other indices.
      for (; OpIt != CE->op_end(); ++OpIt)
	IndexStack.push_back(parseIndex(*OpIt));

      return navigatePointerTreeToRoot(PointerOp);
    }
    else
      return nullptr;
  }
  else if (LoadInst *LI = dyn_cast<LoadInst>(P)) {
    return navigatePointerTreeToRoot(LI->getPointerOperand());
  }
  else if (Argument *A = dyn_cast<Argument>(P)) {
    auto AArg = ArgBindings.find(A);
    if (AArg != ArgBindings.end())
      return navigatePointerTreeToRoot(AArg->second);
    else
      return P;
  }
  else if (AllocaInst *AI = dyn_cast<AllocaInst>(P)) {
    return (isa<StructType>(AI->getAllocatedType())) ? P : nullptr;
  }
  else if (GlobalVariable *GV = dyn_cast<GlobalVariable>(P)) {
    return (GV->getValueType()->isStructTy()) ? P : nullptr;
  }
  return nullptr;
}

StructError *StructTreeWalker::navigateStructTree(StructTree *Root, bool Create) {
  if (isa<StructError>(Root))
    return cast<StructError>(Root);

  StructNode *RN = cast<StructNode>(Root);
  unsigned ChildIdx = IndexStack.back();
  IndexStack.pop_back();

  StructTree *Child = RN->getStructElement(ChildIdx);
  if (Child != nullptr) {
    return navigateStructTree(Child, Create);
  }
  else if (Create) {
    Type *ChildType = RN->getStructType()->getElementType(ChildIdx);
    while (SequentialType *STy = dyn_cast<SequentialType>(ChildType)) {
      // Just discard array indices.
      ChildType = STy->getElementType();
      IndexStack.pop_back();
    }

    if (StructType *ChildST = dyn_cast<StructType>(ChildType)) {
      RN->setStructElement(ChildIdx, new StructNode(ChildST, Root));
      return navigateStructTree(RN->getStructElement(ChildIdx), Create);
    }
    else {
      // TODO: deal with sequential types
      RN->setStructElement(ChildIdx, new StructError(Root));
      return cast<StructError>(RN->getStructElement(ChildIdx));
    }
  }
  else
    return nullptr;
}

unsigned StructTreeWalker::parseIndex(const Use &U) const {
  return cast<ConstantInt>(U.get())->getZExtValue();
}

StructErrorMap::StructErrorMap(const StructErrorMap &M)
  : StructMap(), ArgBindings(M.ArgBindings) {
  for (auto &KV : M.StructMap) {
    std::unique_ptr<StructTree> New(KV.second->clone());
    this->StructMap.insert(std::make_pair(KV.first, std::move(New)));
  }
}

StructErrorMap &StructErrorMap::operator=(const StructErrorMap &O) {
  StructErrorMap Tmp(O);
  std::swap(this->StructMap, Tmp.StructMap);
  std::swap(this->ArgBindings, Tmp.ArgBindings);

  return *this;
}

void StructErrorMap::initArgumentBindings(Function &F,
					  const ArrayRef<Value *> AArgs) {
  auto AArgIt = AArgs.begin();
  for (Argument &FArg : F.args()) {
    if (AArgIt == AArgs.end()) break;
    if (FArg.getType()->isPointerTy()
	&& cast<PointerType>(FArg.getType())->getElementType()->isStructTy())
      ArgBindings.insert(std::make_pair(&FArg, *AArgIt));

    ++AArgIt;
  }
}

void StructErrorMap::setFieldError(Value *P, const StructTree::RangeError &Err) {
  StructTreeWalker STW(ArgBindings);
  Value *RootP = STW.retrieveRootPointer(P);
  if (RootP == nullptr)
    return;

  auto RootIt = StructMap.find(RootP);
  if (RootIt == StructMap.end()) {
    RootIt = StructMap.insert(std::make_pair(RootP, std::unique_ptr<StructTree>(STW.makeRoot(RootP)))).first;
  }

  StructError *FE = STW.getOrCreateFieldNode(RootIt->second.get());
  FE->setError(Err);
}

const StructTree::RangeError *StructErrorMap::getFieldError(Value *P) const {
  StructTreeWalker STW(ArgBindings);
  Value *RootP = STW.retrieveRootPointer(P);
  if (RootP == nullptr)
    return nullptr;

  auto RootIt = StructMap.find(RootP);
  if (RootIt == StructMap.end())
    return nullptr;

  StructError *FE = STW.getFieldNode(RootIt->second.get());
  if (FE)
    return &FE->getError();
  else
    return nullptr;
}

void StructErrorMap::updateStructTree(const StructErrorMap &O, const ArrayRef<Value *> Pointers) {
  StructTreeWalker STW(this->ArgBindings);
  for (Value *P : Pointers) {
    if (Value *Root = STW.retrieveRootPointer(P)) {
      auto OTreeIt = O.StructMap.find(Root);
      if (OTreeIt != O.StructMap.end()
	  && OTreeIt->second != nullptr)
	this->StructMap[Root].reset(OTreeIt->second->clone());
    }
  }
}

} // end namespace ErrorProp

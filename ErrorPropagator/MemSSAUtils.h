#ifndef ERRORPROPAGATOR_MEMSSAUTILS_H
#define ERRORPROPAGATOR_MEMSSAUTILS_H

#include "RangeErrorMap.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"

namespace ErrorProp {

#define DEFAULT_RE_COUNT 2U

class MemSSAUtils {
public:
  typedef llvm::SmallVector<const RangeErrorMap::RangeError *, DEFAULT_RE_COUNT> REVector;

  MemSSAUtils(RangeErrorMap &RMap, MemorySSA &MemSSA)
    : RMap(RMap), MemSSA(MemSSA) {}

  void findMemSSAError(Instruction *I, MemoryAccess *MA);

  REVector &getRangeErrors() { return Res; }

  static Value *getOriginPointer(MemorySSA &MemSSA, Value *Pointer);

private:
  RangeErrorMap &RMap;
  MemorySSA &MemSSA;
  SmallSet<MemoryAccess *, DEFAULT_RE_COUNT> Visited;
  REVector Res;

  void findLOEError(Instruction *I);
  void findMemDefError(Instruction *I, const MemoryDef *MD);
  void findMemPhiError(Instruction *I, MemoryPhi *MPhi);
};

} // end of namespace ErrorProp

#endif

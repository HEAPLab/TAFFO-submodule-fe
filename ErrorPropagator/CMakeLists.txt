add_llvm_library(LLVMErrorPropagator OBJECT BUILDTREE_ONLY
  ErrorPropagator.cpp
  FunctionErrorPropagator.cpp
  RangeErrorMap.cpp
  StructErrorMap.cpp
  FunctionCopyMap.cpp
  Propagators.cpp
  PropagatorsUtils.cpp
  SpecialFunctions.cpp
  MemSSAUtils.cpp
  AffineForms.cpp
  FixedPoint.cpp
)

set_property(TARGET obj.LLVMErrorPropagator PROPERTY POSITION_INDEPENDENT_CODE ON)

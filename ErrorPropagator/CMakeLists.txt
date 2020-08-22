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

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_property(TARGET obj.LLVMErrorPropagator APPEND_STRING PROPERTY
    LINK_FLAGS " -undefined dynamic_lookup")
endif()

set_property(TARGET obj.LLVMErrorPropagator PROPERTY CXX_STANDARD 11)
set_property(TARGET obj.LLVMErrorPropagator PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET obj.LLVMErrorPropagator PROPERTY POSITION_INDEPENDENT_CODE ON)

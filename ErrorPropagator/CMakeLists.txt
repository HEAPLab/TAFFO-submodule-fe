add_llvm_loadable_module(LLVMErrorPropagator
  ErrorPropagator.cpp
  FunctionErrorPropagator.cpp
  RangeErrorMap.cpp
  FunctionCopyMap.cpp
  Propagators.cpp

  PLUGIN_TOOL
  opt
  )

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_property(TARGET LLVMErrorPropagator APPEND_STRING PROPERTY
    LINK_FLAGS " -undefined dynamic_lookup")
endif()

set_property(TARGET LLVMErrorPropagator PROPERTY CXX_STANDARD 11)
set_property(TARGET LLVMErrorPropagator PROPERTY CXX_STANDARD_REQUIRED ON)

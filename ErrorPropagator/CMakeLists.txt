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

# Use C++11 to compile your pass (i.e., supply -std=c++11).
#target_compile_features(LLVMErrorPropagator PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
# set_target_properties(LLVMErrorPropagator PROPERTIES
#     COMPILE_FLAGS "-fno-rtti"
# )

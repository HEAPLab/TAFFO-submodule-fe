add_llvm_loadable_module(LLVMEPUtils
  AffineForms.cpp
  FixedPoint.cpp
  Metadata.cpp

  PLUGIN_TOOL
  opt
  )

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_property(TARGET LLVMEPUtils APPEND_STRING PROPERTY
    LINK_FLAGS " -undefined dynamic_lookup")
endif()

set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD 11)
set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD_REQUIRED ON)

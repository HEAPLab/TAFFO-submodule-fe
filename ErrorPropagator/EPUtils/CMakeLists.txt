add_library(LLVMEPUtils
  Metadata.cpp
  InputInfo.cpp
  ../../include/ErrorPropagator/EPUtils/Metadata.h
  ../../include/ErrorPropagator/EPUtils/InputInfo.h
)

set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD 11)
set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET LLVMEPUtils PROPERTY POSITION_INDEPENDENT_CODE ON)

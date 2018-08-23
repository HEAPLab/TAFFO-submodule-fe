add_library(LLVMMDUtils
  Metadata.cpp
  InputInfo.cpp
  ../../include/ErrorPropagator/MDUtils/Metadata.h
  ../../include/ErrorPropagator/MDUtils/InputInfo.h
)

set_property(TARGET LLVMMDUtils PROPERTY CXX_STANDARD 11)
set_property(TARGET LLVMMDUtils PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET LLVMMDUtils PROPERTY POSITION_INDEPENDENT_CODE ON)

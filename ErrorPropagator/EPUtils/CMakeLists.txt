add_library(LLVMEPUtils
  AffineForms.cpp
  FixedPoint.cpp
  Metadata.cpp
  ../../include/ErrorPropagator/EPUtils/AffineForms.h
  ../../include/ErrorPropagator/EPUtils/FixedPoint.h
  ../../include/ErrorPropagator/EPUtils/Metadata.h
)

set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD 11)
set_property(TARGET LLVMEPUtils PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET LLVMEPUtils PROPERTY POSITION_INDEPENDENT_CODE ON)

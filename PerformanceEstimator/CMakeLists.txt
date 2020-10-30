find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_python3_module(zipapp REQUIRED)

add_custom_command(
  OUTPUT
  taffo-pe
  
  COMMAND 
  "${Python3_EXECUTABLE}" "-m" "zipapp"
  "${CMAKE_CURRENT_SOURCE_DIR}" "-p" "${Python3_EXECUTABLE}" "-m" "main_pe:main" "-o" "taffo-pe" 
  )
add_custom_command(
  OUTPUT
  taffo-pe-train-class
  
  COMMAND 
  "${Python3_EXECUTABLE}" "-m" "zipapp"
  "${CMAKE_CURRENT_SOURCE_DIR}" "-p" "${Python3_EXECUTABLE}" "-m" "main_pe_train_class:main" "-o" "taffo-pe-train-class" 
  )
add_custom_command(
  OUTPUT
  taffo-pe-train-regr
  
  COMMAND 
  "${Python3_EXECUTABLE}" "-m" "zipapp"
  "${CMAKE_CURRENT_SOURCE_DIR}" "-p" "${Python3_EXECUTABLE}" "-m" "main_pe_train_regr:main" "-o" "taffo-pe-train-regr" 
  )
  
add_custom_target(PerformanceEstimator
  ALL
  
  DEPENDS
  taffo-pe
  taffo-pe-train-class
  taffo-pe-train-regr
  )

install(
  FILES 
  ${CMAKE_CURRENT_BINARY_DIR}/taffo-pe
  ${CMAKE_CURRENT_BINARY_DIR}/taffo-pe-train-class
  ${CMAKE_CURRENT_BINARY_DIR}/taffo-pe-train-regr
  
  DESTINATION
  bin 
  
  PERMISSIONS 
  OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ
  WORLD_EXECUTE WORLD_READ
  )

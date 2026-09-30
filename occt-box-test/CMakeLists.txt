cmake_minimum_required(VERSION 3.20)
project(occt_box_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCASCADE CONFIG REQUIRED
  COMPONENTS FoundationClasses ModelingData ModelingAlgorithms)

add_executable(occt_box main.cpp)
target_include_directories(occt_box PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
target_link_libraries(occt_box PRIVATE TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo TKTopAlgo TKPrim)

if(WIN32)
  add_custom_command(TARGET occt_box POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${OpenCASCADE_BINARY_DIR}/libTKernel.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKMath.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKG2d.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKG3d.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKGeomBase.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKBRep.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKGeomAlgo.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKTopAlgo.dll"
      "${OpenCASCADE_BINARY_DIR}/libTKPrim.dll"
      "$<TARGET_FILE_DIR:occt_box>")
endif()

cmake_minimum_required(VERSION 3.20)
project(occt_lattice_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCASCADE CONFIG REQUIRED
  COMPONENTS FoundationClasses ModelingData ModelingAlgorithms)

add_executable(occt_lattice main.cpp)
target_include_directories(occt_lattice PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
target_link_libraries(occt_lattice PRIVATE
  TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo TKTopAlgo TKBO)

if(WIN32)
  set(OCCT_RUNTIME_LIBS
    TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo TKTopAlgo TKBO)
  foreach(lib IN LISTS OCCT_RUNTIME_LIBS)
    add_custom_command(TARGET occt_lattice POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OpenCASCADE_BINARY_DIR}/lib${lib}.dll"
        "$<TARGET_FILE_DIR:occt_lattice>")
  endforeach()
endif()


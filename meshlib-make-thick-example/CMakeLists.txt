cmake_minimum_required(VERSION 3.21)
project(MeshLibMakeThickExample VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MESHLIB_ROOT "E:/CLibrary/MeshLibDistVS22_v3.1.4.297" CACHE PATH "MeshLib VS22 root")
set(MESHLIB_INCLUDE_DIR "${MESHLIB_ROOT}/install/include")
set(MESHLIB_LIBRARY_DIR "${MESHLIB_ROOT}/install/lib/Release")
set(MESHLIB_RUNTIME_DIR "${MESHLIB_ROOT}/install/app/Release")

if(NOT EXISTS "${MESHLIB_INCLUDE_DIR}/MRMesh/MROffsetVerts.h")
    message(FATAL_ERROR "MeshLib headers were not found under ${MESHLIB_INCLUDE_DIR}")
endif()

add_executable(MakeThickMeshExample main.cpp)
target_include_directories(MakeThickMeshExample PRIVATE "${MESHLIB_INCLUDE_DIR}")
target_link_directories(MakeThickMeshExample PRIVATE "${MESHLIB_LIBRARY_DIR}")
target_link_libraries(MakeThickMeshExample PRIVATE MRMesh)

if(MSVC)
    target_compile_options(MakeThickMeshExample PRIVATE /bigobj /utf-8)
    target_compile_definitions(MakeThickMeshExample PRIVATE _ITERATOR_DEBUG_LEVEL=0)
    set_property(TARGET MakeThickMeshExample PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

file(GLOB MESHLIB_RUNTIME_DLLS CONFIGURE_DEPENDS "${MESHLIB_RUNTIME_DIR}/*.dll")
add_custom_command(TARGET MakeThickMeshExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MESHLIB_RUNTIME_DLLS} "$<TARGET_FILE_DIR:MakeThickMeshExample>"
    COMMAND_EXPAND_LISTS
    COMMENT "Copying MeshLib runtime DLLs"
)

enable_testing()
add_test(
    NAME MakeThickMeshDefaultPlane
    COMMAND MakeThickMeshExample "" "${CMAKE_CURRENT_BINARY_DIR}/test-output" 0.10 0.05
)

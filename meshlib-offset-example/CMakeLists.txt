cmake_minimum_required(VERSION 3.21)
project(MeshLibOffsetExample VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MESHLIB_ROOT "E:/CLibrary/MeshLibDistVS22_v3.1.4.297" CACHE PATH "MeshLib VS22 root")
set(MESHLIB_INCLUDE_DIR "${MESHLIB_ROOT}/install/include")
set(MESHLIB_LIBRARY_DIR "${MESHLIB_ROOT}/install/lib/Release")
set(MESHLIB_RUNTIME_DIR "${MESHLIB_ROOT}/install/app/Release")

if(NOT EXISTS "${MESHLIB_INCLUDE_DIR}/MRMesh/MRMesh.h")
    message(FATAL_ERROR "MeshLib headers were not found under ${MESHLIB_INCLUDE_DIR}")
endif()

add_executable(MeshOffsetExample main.cpp)
target_include_directories(MeshOffsetExample PRIVATE "${MESHLIB_INCLUDE_DIR}")
target_link_directories(MeshOffsetExample PRIVATE "${MESHLIB_LIBRARY_DIR}")
target_link_libraries(MeshOffsetExample PRIVATE MRVoxels MRMesh)

if(MSVC)
    target_compile_options(MeshOffsetExample PRIVATE /bigobj /utf-8)
    target_compile_definitions(MeshOffsetExample PRIVATE _ITERATOR_DEBUG_LEVEL=0)
    set_property(TARGET MeshOffsetExample PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

file(GLOB MESHLIB_RUNTIME_DLLS CONFIGURE_DEPENDS "${MESHLIB_RUNTIME_DIR}/*.dll")
add_custom_command(TARGET MeshOffsetExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MESHLIB_RUNTIME_DLLS} "$<TARGET_FILE_DIR:MeshOffsetExample>"
    COMMAND_EXPAND_LISTS
    COMMENT "Copying MeshLib runtime DLLs"
)

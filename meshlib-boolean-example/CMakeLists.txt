cmake_minimum_required(VERSION 3.21)
project(MeshLibBooleanExample VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MESHLIB_ROOT "E:/CLibrary/MeshLib-3.1.4.297-msvc2022-x64"
    CACHE PATH "Path to the prebuilt MeshLib package")
set(MESHLIB_INCLUDE_DIR "${MESHLIB_ROOT}/install/include")
set(MESHLIB_LIBRARY_DIR "${MESHLIB_ROOT}/install/lib/Release")
set(MESHLIB_RUNTIME_DIR "${MESHLIB_ROOT}/install/app/Release")

if(NOT EXISTS "${MESHLIB_INCLUDE_DIR}/MRMesh/MRMesh.h")
    message(FATAL_ERROR "MeshLib headers were not found in ${MESHLIB_INCLUDE_DIR}")
endif()

add_executable(MeshBooleanExample main.cpp)
target_include_directories(MeshBooleanExample PRIVATE "${MESHLIB_INCLUDE_DIR}")
target_link_directories(MeshBooleanExample PRIVATE "${MESHLIB_LIBRARY_DIR}")
target_link_libraries(MeshBooleanExample PRIVATE MRMesh)

if(MSVC)
    target_compile_options(MeshBooleanExample PRIVATE /bigobj /utf-8)
    target_compile_definitions(MeshBooleanExample PRIVATE _ITERATOR_DEBUG_LEVEL=0)
    set_property(TARGET MeshBooleanExample PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# Copy MeshLib and third-party runtime DLLs next to the executable.
file(GLOB MESHLIB_RUNTIME_DLLS CONFIGURE_DEPENDS "${MESHLIB_RUNTIME_DIR}/*.dll")
add_custom_command(TARGET MeshBooleanExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MESHLIB_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:MeshBooleanExample>"
    COMMAND_EXPAND_LISTS
    COMMENT "Copying MeshLib runtime DLLs"
)

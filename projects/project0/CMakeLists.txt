cmake_minimum_required(VERSION 3.25)

project(project0_device_query LANGUAGES CXX CUDA)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)

    find_package(CUDAToolkit REQUIRED)
endif()

set(PROJECT0_BASE_FILES
    common/utility.h
    gpu/driver_utility.h
    gpu/utility.h)

set(PROJECT0_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/base")
if(NOT IS_DIRECTORY "${PROJECT0_BASE_DIR}")
    get_filename_component(PROJECT0_BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}/../../base" ABSOLUTE)
endif()

set(PROJECT0_BASE_HEADERS)
foreach(PROJECT0_BASE_FILE IN LISTS PROJECT0_BASE_FILES)
    if(NOT EXISTS "${PROJECT0_BASE_DIR}/${PROJECT0_BASE_FILE}")
        message(FATAL_ERROR "Unable to find base file: ${PROJECT0_BASE_FILE}")
    endif()
    list(APPEND PROJECT0_BASE_HEADERS "${PROJECT0_BASE_DIR}/${PROJECT0_BASE_FILE}")
endforeach()

file(GLOB_RECURSE PROJECT0_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu")
file(GLOB_RECURSE PROJECT0_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cuh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.inl")

add_executable(project0 ${PROJECT0_SOURCES} ${PROJECT0_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT0_SOURCES} ${PROJECT0_HEADERS})
target_sources(project0 PRIVATE ${PROJECT0_BASE_HEADERS})
source_group(TREE "${PROJECT0_BASE_DIR}" PREFIX "base" FILES ${PROJECT0_BASE_HEADERS})
target_include_directories(project0 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(project0 PRIVATE "${PROJECT0_BASE_DIR}")
target_link_libraries(project0 PRIVATE CUDA::cudart CUDA::cuda_driver CUDA::nvml)

# Add shared files manually with target_sources() when this project uses them.
# The same list controls what submit_project0 bundles from base/.
if(COMMAND add_project_submission_target)
    add_project_submission_target(
        NAME project0
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BASE_FILES ${PROJECT0_BASE_FILES})
endif()

cmake_minimum_required(VERSION 3.25)

project(project3_alexnet LANGUAGES CXX CUDA)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)
    if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ProjectSubmission.cmake" OPTIONAL)
endif()

find_package(CUDAToolkit REQUIRED)

set(PROJECT3_BASE_FILES
    common/timer.h
    gpu/buffer/buffer.h
    gpu/buffer/buffer_storage.h
    gpu/buffer/pinned_buffer.h
    gpu/event.h
    gpu/stream.h
    gpu/utility.h)
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/base")
    set(PROJECT3_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/base")
else()
    get_filename_component(PROJECT3_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../base" ABSOLUTE)
endif()
set(PROJECT3_BASE_PATHS)
foreach(file IN LISTS PROJECT3_BASE_FILES)
    list(APPEND PROJECT3_BASE_PATHS "${PROJECT3_BASE_DIR}/${file}")
endforeach()

set(PROJECT3_FILES
    src/main.cpp
    src/alexnet_common.cpp
    src/alexnet_cpu.cpp
    src/alexnet_gpu.cu
    src/image.cpp
    src/evaluation.cpp
    include/alexnet_common.h
    include/alexnet_cpu.h
    include/alexnet_gpu.h
    include/image.h
    include/evaluation.h)

add_executable(project3 ${PROJECT3_FILES} ${PROJECT3_BASE_PATHS})
target_include_directories(project3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${PROJECT3_BASE_DIR}")
target_link_libraries(project3 PRIVATE CUDA::cudart CUDA::nvtx3)
# Keep optimized code and source correlation for Nsight Compute; do not use -G.
target_compile_options(project3 PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-lineinfo>)
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET project3 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:project3>/assets"
        VERBATIM)
endif()
if(WIN32)
    target_compile_definitions(project3 PRIVATE NOMINMAX)
    target_compile_options(project3 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
    target_compile_options(project3 PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8>)
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    # Resolve relative command-line paths from the repository root, or the extracted package root.
    get_filename_component(PROJECT3_WORKING_DIRECTORY "${PROJECT3_BASE_DIR}/.." ABSOLUTE)
    set_property(TARGET project3 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT3_WORKING_DIRECTORY}")
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT3_FILES})
source_group(TREE "${PROJECT3_BASE_DIR}" PREFIX "base" FILES ${PROJECT3_BASE_PATHS})

if(COMMAND add_project_submission_target)
    add_project_submission_target(NAME project3 SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
                                  BASE_FILES ${PROJECT3_BASE_FILES} EXCLUDE_SCRIPTS)
endif()

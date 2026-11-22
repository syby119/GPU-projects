cmake_minimum_required(VERSION 3.25)

project(project1_mandelbrot LANGUAGES CXX CUDA)

option(PROJECT1_OFFLINE_ONLY
       "Build project1 without OpenGL/GLFW online rendering"
       OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)

    find_package(CUDAToolkit REQUIRED)
endif()

find_path(STB_IMAGE_WRITE_INCLUDE_DIR NAMES stb_image_write.h REQUIRED)

set(PROJECT1_BASE_FILES
    common/timer.h
    common/utility.h
    gpu/buffer/buffer.h
    gpu/buffer/buffer_storage.h
    gpu/timer.h
    gpu/utility.h)

if(NOT PROJECT1_OFFLINE_ONLY)
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)

    list(APPEND PROJECT1_BASE_FILES
        common/application.cpp
        common/application.h
        common/enum.h
        common/event/event.h
        common/event/event_dispatcher.h
        common/event/event_queue.h
        common/event/keyboard_event.h
        common/event/mouse_event.h
        common/event/window_event.h
        common/input/input.cpp
        common/input/input.h
        common/input/input_mapping.cpp
        common/input/input_mapping.h
        common/window.cpp
        common/window.h
        gpu/graphics_resource/graphics_buffer_resource.h
        gpu/graphics_resource/graphics_resource.h
        graphics/buffer.h
        graphics/buffer.cpp
        graphics/gl_utility.h
        graphics/graphics_context.cpp
        graphics/graphics_context.h
        graphics/rhi.h
        graphics/rhi.cpp
        graphics/gl_program.h
        graphics/gl_program.cpp
        graphics/rhi_resource.h
        graphics/shader_module.h
        graphics/shader_module.cpp
        graphics/vertex_array.h
        graphics/vertex_array.cpp
        graphics/viewport.h)
endif()

set(PROJECT1_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/base")
if(NOT IS_DIRECTORY "${PROJECT1_BASE_DIR}")
    get_filename_component(PROJECT1_BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}/../../base" ABSOLUTE)
endif()

set(PROJECT1_BASE_SOURCES)
set(PROJECT1_BASE_HEADERS)
foreach(PROJECT1_BASE_FILE IN LISTS PROJECT1_BASE_FILES)
    set(PROJECT1_BASE_FILE_PATH "${PROJECT1_BASE_DIR}/${PROJECT1_BASE_FILE}")
    if(NOT EXISTS "${PROJECT1_BASE_FILE_PATH}")
        message(FATAL_ERROR "Unable to find base file: ${PROJECT1_BASE_FILE}")
    endif()

    if(PROJECT1_BASE_FILE_PATH MATCHES "\\.(c|cc|cpp|cxx|cu)$")
        list(APPEND PROJECT1_BASE_SOURCES "${PROJECT1_BASE_FILE_PATH}")
    else()
        list(APPEND PROJECT1_BASE_HEADERS "${PROJECT1_BASE_FILE_PATH}")
    endif()
endforeach()

file(GLOB_RECURSE PROJECT1_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu")

if(PROJECT1_OFFLINE_ONLY)
    list(REMOVE_ITEM PROJECT1_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/mandelbrot_compute.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/mandelbrot_viewer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/playback_controller.cpp")
endif()

file(GLOB_RECURSE PROJECT1_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cuh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.inl")

add_executable(project1 ${PROJECT1_SOURCES} ${PROJECT1_HEADERS})
set_target_properties(project1 PROPERTIES CUDA_ARCHITECTURES native)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT1_SOURCES} ${PROJECT1_HEADERS})
target_sources(project1 PRIVATE ${PROJECT1_BASE_SOURCES} ${PROJECT1_BASE_HEADERS})
source_group(TREE "${PROJECT1_BASE_DIR}" PREFIX "base" FILES ${PROJECT1_BASE_SOURCES} ${PROJECT1_BASE_HEADERS})
target_include_directories(project1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(project1 PRIVATE "${PROJECT1_BASE_DIR}")
target_include_directories(project1 PRIVATE "${STB_IMAGE_WRITE_INCLUDE_DIR}")
target_link_libraries(project1 PRIVATE CUDA::cudart)
if(PROJECT1_OFFLINE_ONLY)
    target_compile_definitions(project1 PRIVATE PROJECT1_OFFLINE_ONLY)
else()
    target_link_libraries(project1 PRIVATE glad::glad glfw glm::glm)
endif()

if(WIN32 AND NOT PROJECT1_OFFLINE_ONLY)
    add_custom_command(TARGET project1 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                $<TARGET_RUNTIME_DLLS:project1> "$<TARGET_FILE_DIR:project1>"
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mandelbrot.cu"
    PROPERTIES COMPILE_OPTIONS "--expt-relaxed-constexpr")
set_target_properties(project1 PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

if(COMMAND add_project_submission_target)
    add_project_submission_target(
        NAME project1
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BASE_FILES ${PROJECT1_BASE_FILES})
endif()

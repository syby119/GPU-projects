cmake_minimum_required(VERSION 3.25)

project(project2_optix_ray_tracer LANGUAGES CXX CUDA)

option(PROJECT2_OFFLINE_ONLY "Build project2 without OpenGL/GLFW window rendering" OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

find_package(CUDAToolkit REQUIRED)
if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()
find_path(OPTIX_INCLUDE_DIR NAMES optix.h REQUIRED)
find_path(STB_IMAGE_WRITE_INCLUDE_DIR NAMES stb_image_write.h REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

if(NOT PROJECT2_OFFLINE_ONLY)
    find_package(glad CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base")
    set(PROJECT2_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/base")
else()
    set(PROJECT2_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../base")
endif()
set(PROJECT2_OPTIX_FILES
    optix/utility.h
    optix/device_context.h
    optix/device_context.cpp
    optix/module.h
    optix/module.cpp
    optix/program_group.h
    optix/program_group.cpp
    optix/pipeline.h
    optix/pipeline.cpp
    optix/detail/acceleration_structure.h
    optix/detail/acceleration_structure.cpp
    optix/geometry_acceleration_structure.h
    optix/geometry_acceleration_structure.cpp
    optix/instance_acceleration_structure.h
    optix/instance_acceleration_structure.cpp
    optix/static_transform.h
    optix/static_transform.cpp
    optix/denoiser.h
    optix/denoiser.cpp
    optix/shader_binding_table.h
    optix/shader_binding_table.cpp)
set(PROJECT2_BASE_FILES
    gpu/buffer/buffer.h
    gpu/buffer/buffer_storage.h
    gpu/timer.h
    gpu/utility.h
    gpu/texture/texture2d.h
    gpu/texture/texture2d.cpp
    gpu/texture/texture_cubemap.h
    gpu/texture/texture_cubemap.cpp
    ${PROJECT2_OPTIX_FILES})

if(NOT PROJECT2_OFFLINE_ONLY)
    list(APPEND PROJECT2_BASE_FILES
        common/application.cpp
        common/application.h
        common/window.cpp
        common/window.h
        common/timer.h
        common/enum.h
        common/input/input.cpp
        common/input/input.h
        common/input/input_mapping.cpp
        common/input/input_mapping.h
        common/event/event.h
        common/event/event_dispatcher.h
        common/event/event_queue.h
        common/event/keyboard_event.h
        common/event/mouse_event.h
        common/event/window_event.h
        gpu/graphics_resource/graphics_resource.h
        gpu/graphics_resource/graphics_buffer_resource.h
        graphics/buffer.cpp
        graphics/buffer.h
        graphics/graphics_context.cpp
        graphics/graphics_context.h
        graphics/gl_utility.h
        graphics/gl_program.cpp
        graphics/gl_program.h
        graphics/rhi.cpp
        graphics/rhi.h
        graphics/rhi_resource.h
        graphics/shader_module.cpp
        graphics/shader_module.h
        graphics/vertex_array.cpp
        graphics/vertex_array.h
        graphics/viewport.h)
endif()

set(PROJECT2_BASE_PATHS)
foreach(file IN LISTS PROJECT2_BASE_FILES)
    list(APPEND PROJECT2_BASE_PATHS "${PROJECT2_BASE_DIR}/${file}")
endforeach()

# Prefer the compute capability of GPU 0 when it is visible at configure time.
# Fall back to compute_75 so configuration still works without an accessible GPU.
set(PROJECT2_OPTIX_GPU_ARCH "compute_75")
find_program(PROJECT2_NVIDIA_SMI_EXECUTABLE NAMES nvidia-smi
             HINTS "/usr/lib/wsl/lib")
if(PROJECT2_NVIDIA_SMI_EXECUTABLE)
    execute_process(
        COMMAND "${PROJECT2_NVIDIA_SMI_EXECUTABLE}" --id=0 --query-gpu=compute_cap
                --format=csv,noheader
        RESULT_VARIABLE PROJECT2_NVIDIA_SMI_RESULT
        OUTPUT_VARIABLE PROJECT2_GPU_COMPUTE_CAP
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(STRIP "${PROJECT2_GPU_COMPUTE_CAP}" PROJECT2_GPU_COMPUTE_CAP)

    if("${PROJECT2_NVIDIA_SMI_RESULT}" STREQUAL "0" AND
       "${PROJECT2_GPU_COMPUTE_CAP}" MATCHES "^[0-9]+\\.[0-9]+$")
        string(REPLACE "." "" PROJECT2_GPU_COMPUTE_CAP_SUFFIX
               "${PROJECT2_GPU_COMPUTE_CAP}")
        set(PROJECT2_OPTIX_GPU_ARCH "compute_${PROJECT2_GPU_COMPUTE_CAP_SUFFIX}")
        message(STATUS "GPU 0 compute capability: ${PROJECT2_GPU_COMPUTE_CAP}")
        message(STATUS "Using OptiX IR target ${PROJECT2_OPTIX_GPU_ARCH}")
    else()
        message(STATUS "Could not query GPU 0 compute capability; using compute_75")
    endif()
else()
    message(STATUS "nvidia-smi not found; using compute_75")
endif()

set(PROJECT2_OPTIX_IR "${CMAKE_CURRENT_BINARY_DIR}/assets/optix/project2.optixir"
    CACHE FILEPATH "Output path for the generated Project 2 OptiX IR")
get_filename_component(PROJECT2_OPTIX_IR "${PROJECT2_OPTIX_IR}" ABSOLUTE
                       BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
get_filename_component(PROJECT2_OPTIX_IR_DIRECTORY "${PROJECT2_OPTIX_IR}" DIRECTORY)
add_custom_command(
    OUTPUT "${PROJECT2_OPTIX_IR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT2_OPTIX_IR_DIRECTORY}"
    COMMAND "${CUDAToolkit_NVCC_EXECUTABLE}" --optix-ir --std=c++17
            "--gpu-architecture=${PROJECT2_OPTIX_GPU_ARCH}"
            "-I${OPTIX_INCLUDE_DIR}"
            "-I${CMAKE_CURRENT_SOURCE_DIR}/include"
            -o "${PROJECT2_OPTIX_IR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/optix_programs.cu"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/optix_programs.cu"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/launch_params.h"
    VERBATIM)
add_custom_target(project2_optix_ir DEPENDS "${PROJECT2_OPTIX_IR}")
set_target_properties(project2_optix_ir PROPERTIES FOLDER "utility")

add_executable(project2
    src/camera.cpp
    src/camera_controller.cpp
    src/main.cpp
    src/optix_offline_render.cpp
    src/optix_renderer.cpp
    src/scene.cpp
    include/camera.h
    include/camera_controller.h
    include/launch_params.h
    include/optix_offline_render.h
    include/optix_renderer.h
    include/scene.h
    ${PROJECT2_BASE_PATHS})
if(PROJECT2_OFFLINE_ONLY)
    target_compile_definitions(project2 PRIVATE PROJECT2_OFFLINE_ONLY)
else()
    target_sources(project2 PRIVATE src/optix_viewer.cpp include/optix_viewer.h)
endif()
add_dependencies(project2 project2_optix_ir)
target_include_directories(project2 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT2_BASE_DIR}"
    "${OPTIX_INCLUDE_DIR}"
    "${STB_IMAGE_WRITE_INCLUDE_DIR}")
target_link_libraries(project2 PRIVATE
    CUDA::cudart tinyobjloader::tinyobjloader glm::glm ${CMAKE_DL_LIBS})
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET project2 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:project2>/assets"
        VERBATIM)
endif()
add_custom_command(TARGET project2 POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:project2>/assets/optix"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${PROJECT2_OPTIX_IR}" "$<TARGET_FILE_DIR:project2>/assets/optix/project2.optixir"
    VERBATIM)
if(NOT PROJECT2_OFFLINE_ONLY)
    target_link_libraries(project2 PRIVATE glad::glad glfw)
endif()

if(WIN32 AND NOT PROJECT2_OFFLINE_ONLY)
    target_link_libraries(project2 PRIVATE Cfgmgr32)
    add_custom_command(TARGET project2 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                $<TARGET_RUNTIME_DLLS:project2> "$<TARGET_FILE_DIR:project2>"
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()

if(COMMAND add_project_submission_target)
    add_project_submission_target(
        NAME project2
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BASE_FILES ${PROJECT2_BASE_FILES})
endif()

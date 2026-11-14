#pragma once

#include <cuda.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#define LOG_CU(call) ::gpu::logCudaDriverErrorImpl((call), #call, __FILE__, __LINE__)
#define CHECK_CU(call) ::gpu::checkCudaDriverImpl((call), #call, __FILE__, __LINE__)
#define FATAL_CU(call) ::gpu::fatalCudaDriverImpl((call), #call, __FILE__, __LINE__)

namespace gpu {

inline void logCudaDriverErrorImpl(CUresult status, char const* call, char const* file, uint32_t line) {
    if (status != CUDA_SUCCESS) {
        char const* error = nullptr;
        cuGetErrorString(status, &error);
        if (error == nullptr) {
            error = "Unknown CUDA driver error";
        }

        std::fprintf(
            stderr, "[CUDA Driver] %s failed(%d) at %s:%u: %s\n", call, static_cast<int>(status), file, line, error);
    }
}

inline void checkCudaDriverImpl(CUresult status, char const* call, char const* file, uint32_t line) {
    if (status != CUDA_SUCCESS) {
        char const* error = nullptr;
        cuGetErrorString(status, &error);
        if (error == nullptr) {
            error = "Unknown CUDA driver error";
        }

        std::fprintf(stderr, "[CUDA Driver] %s failed at %s:%u: %s\n", call, file, line, error);

        char message[256];
        std::snprintf(message, sizeof(message), "CUDA driver error(%d): %s", static_cast<int>(status), error);
        throw std::runtime_error(message);
    }
}

inline void fatalCudaDriverImpl(CUresult status, char const* call, char const* file, uint32_t line) {
    if (status != CUDA_SUCCESS) {
        logCudaDriverErrorImpl(status, call, file, line);
        std::exit(EXIT_FAILURE);
    }
}

} // namespace gpu

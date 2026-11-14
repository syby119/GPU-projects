#pragma once

#include <cuda_runtime.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>

#define LOG_CUDA(call) ::gpu::logCudaErrorImpl((call), #call, __FILE__, __LINE__)
#define CHECK_CUDA(call) ::gpu::checkCudaImpl((call), #call, __FILE__, __LINE__)
#define FATAL_CUDA(call) ::gpu::fatalCudaImpl((call), #call, __FILE__, __LINE__)

template <typename T, typename U>
constexpr auto divUp(T x, U y) {
    static_assert(std::is_integral<T>::value, "divUp requires integral type for x");
    static_assert(std::is_integral<U>::value, "divUp requires integral type for y");

    using R = typename std::common_type_t<T, U>;

    R a = static_cast<R>(x);
    R b = static_cast<R>(y);

    return a / b + (a % b != 0);
}

namespace gpu {

inline void logCudaErrorImpl(cudaError_t status, char const* call, char const* file, uint32_t line) {
    if (status != cudaSuccess) {
        std::fprintf(
            stderr,
            "[CUDA] %s failed(%d) at %s:%u: %s\n",
            call,
            static_cast<int>(status),
            file,
            line,
            cudaGetErrorString(status));
    }
}

inline void checkCudaImpl(cudaError_t status, char const* call, char const* file, uint32_t line) {
    if (status != cudaSuccess) {
        std::fprintf(stderr, "[CUDA] %s failed at %s:%u: %s\n", call, file, line, cudaGetErrorString(status));

        char message[256];
        std::snprintf(
            message, sizeof(message), "CUDA runtime error(%d): %s", static_cast<int>(status), cudaGetErrorName(status));
        throw std::runtime_error(message);
    }
}

inline void fatalCudaImpl(cudaError_t status, char const* call, char const* file, uint32_t line) {
    if (status != cudaSuccess) {
        logCudaErrorImpl(status, call, file, line);
        std::exit(EXIT_FAILURE);
    }
}

inline void printCudaDeviceName(int index) {
    cudaDeviceProp properties{};
    LOG_CUDA(cudaGetDeviceProperties(&properties, index));

    std::printf("Using CUDA device %d: %s\n", index, properties.name);
}

} // namespace gpu

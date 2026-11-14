#pragma once

#include <optix.h>
#include <optix_stubs.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#define CHECK_OPTIX(call) ::optix::checkOptixImpl((call), #call, __FILE__, __LINE__)

#define LOG_OPTIX(call) ::optix::logOptixErrorImpl((call), #call, __FILE__, __LINE__)

namespace optix {

inline void checkOptixImpl(OptixResult result, char const* operation, char const* file, uint32_t line) {
    if (result != OPTIX_SUCCESS) {
        std::fprintf(
            stderr,
            "[OptiX] %s failed at %s:%u: %s (%d)\n",
            operation,
            file,
            line,
            optixGetErrorName(result),
            static_cast<int>(result));
        std::string message = std::string(operation) + ": " + optixGetErrorName(result);
        throw std::runtime_error(message);
    }
}

inline void logOptixErrorImpl(OptixResult result, char const* operation, char const* file, uint32_t line) noexcept {
    if (result != OPTIX_SUCCESS) {
        std::fprintf(
            stderr,
            "[OptiX] %s failed at %s:%u: %s (%d)\n",
            operation,
            file,
            line,
            optixGetErrorName(result),
            static_cast<int>(result));
    }
}

template <typename T>
CUdeviceptr address(T const* pointer) noexcept {
    return reinterpret_cast<CUdeviceptr>(pointer);
}

} // namespace optix

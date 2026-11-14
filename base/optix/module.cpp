#include <optix/module.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace optix {

Module Module::adopt(OptixModule handle) noexcept {
    Module module;
    module.m_handle = handle;
    return module;
}

Module::Module(
    DeviceContext const& context,
    OptixModuleCompileOptions const& moduleOptions,
    OptixPipelineCompileOptions const& pipelineOptions,
    std::span<char const> input) {
    OptixResult const result = optixModuleCreate(
        context.handle(), &moduleOptions, &pipelineOptions, input.data(), input.size(), nullptr, nullptr, &m_handle);
    if (result != OPTIX_SUCCESS) {
        throw std::runtime_error(std::string("optixModuleCreate failed: ") + optixGetErrorName(result));
    }
}

Module::~Module() noexcept {
    if (m_handle) {
        LOG_OPTIX(optixModuleDestroy(m_handle));
    }
}

Module::Module(Module&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {}

Module& Module::operator=(Module&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            LOG_OPTIX(optixModuleDestroy(m_handle));
        }
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    return *this;
}

} // namespace optix

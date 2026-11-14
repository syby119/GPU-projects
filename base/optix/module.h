#pragma once

#include <optix/device_context.h>

#include <span>

namespace optix {

class Module {
public:
    Module() = default;

    static Module adopt(OptixModule handle) noexcept;

    Module(
        DeviceContext const& context,
        OptixModuleCompileOptions const& moduleOptions,
        OptixPipelineCompileOptions const& pipelineOptions,
        std::span<char const> input);

    Module(Module&& other) noexcept;

    ~Module() noexcept;

    Module& operator=(Module&& other) noexcept;

    OptixModule handle() const noexcept {
        return m_handle;
    }

private:
    OptixModule m_handle{};
};

} // namespace optix

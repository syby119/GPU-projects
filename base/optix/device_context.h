#pragma once

#include <optix/utility.h>

#include <cstdint>

namespace optix {

class DeviceContext {
public:
    // Requires optixInit() and an active CUDA context on the calling thread.
    explicit DeviceContext(OptixDeviceContextOptions const* options = nullptr);

    ~DeviceContext() noexcept;

    DeviceContext(DeviceContext&& other) noexcept;

    DeviceContext& operator=(DeviceContext&& other) noexcept;

    // Returns 0 if RT cores are unsupported, or major * 10 + minor otherwise.
    uint32_t getRtCoreVersion() const;

    OptixDeviceContext handle() const noexcept {
        return m_handle;
    }

private:
    OptixDeviceContext m_handle{};
};

} // namespace optix

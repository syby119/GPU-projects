#pragma once

#include <optix/device_context.h>

namespace optix {

class ProgramGroup {
public:
    ProgramGroup() = default;

    ProgramGroup(
        DeviceContext const& context,
        OptixProgramGroupDesc const& description,
        OptixProgramGroupOptions const& options);

    ~ProgramGroup() noexcept;

    ProgramGroup(ProgramGroup&& other) noexcept;

    ProgramGroup& operator=(ProgramGroup&& other) noexcept;

    OptixProgramGroup handle() const noexcept {
        return m_handle;
    }

private:
    OptixProgramGroup m_handle{};
};

} // namespace optix

#include <optix/program_group.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace optix {

ProgramGroup::ProgramGroup(
    DeviceContext const& context,
    OptixProgramGroupDesc const& description,
    OptixProgramGroupOptions const& options) {
    OptixResult const result =
        optixProgramGroupCreate(context.handle(), &description, 1, &options, nullptr, nullptr, &m_handle);
    if (result != OPTIX_SUCCESS) {
        throw std::runtime_error(std::string("optixProgramGroupCreate failed: ") + optixGetErrorName(result));
    }
}

ProgramGroup::~ProgramGroup() noexcept {
    if (m_handle) {
        LOG_OPTIX(optixProgramGroupDestroy(m_handle));
    }
}

ProgramGroup::ProgramGroup(ProgramGroup&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {}

ProgramGroup& ProgramGroup::operator=(ProgramGroup&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            LOG_OPTIX(optixProgramGroupDestroy(m_handle));
        }
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    return *this;
}

} // namespace optix

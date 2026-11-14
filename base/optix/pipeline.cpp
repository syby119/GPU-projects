#include <optix/pipeline.h>

#include <optix/shader_binding_table.h>

#include <utility>
#include <vector>

namespace optix {

Pipeline::Pipeline(
    DeviceContext const& context,
    OptixPipelineCompileOptions const& compileOptions,
    OptixPipelineLinkOptions const& linkOptions,
    std::span<ProgramGroup const> groups) {
    std::vector<OptixProgramGroup> handles;
    handles.reserve(groups.size());
    for (ProgramGroup const& group : groups) {
        handles.push_back(group.handle());
    }

    CHECK_OPTIX(optixPipelineCreate(
        context.handle(),
        &compileOptions,
        &linkOptions,
        handles.data(),
        static_cast<uint32_t>(handles.size()),
        nullptr,
        nullptr,
        &m_handle));
}

Pipeline::~Pipeline() noexcept {
    if (m_handle) {
        LOG_OPTIX(optixPipelineDestroy(m_handle));
    }
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            LOG_OPTIX(optixPipelineDestroy(m_handle));
        }
        m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
}

void Pipeline::setStackSize(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    CHECK_OPTIX(optixPipelineSetStackSize(m_handle, a, b, c, d));
}

void Pipeline::launch(
    ShaderBindingTable const& sbt,
    CUdeviceptr params,
    size_t paramsSize,
    cudaStream_t stream,
    uint32_t width,
    uint32_t height,
    uint32_t depth) const {
    CHECK_OPTIX(optixLaunch(m_handle, stream, params, paramsSize, &sbt.table(), width, height, depth));
}

} // namespace optix

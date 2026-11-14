#pragma once

#include <optix/device_context.h>
#include <optix/program_group.h>
#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace optix {

class ShaderBindingTable;

class Pipeline {
public:
    Pipeline() = default;

    Pipeline(
        DeviceContext const& context,
        OptixPipelineCompileOptions const& compileOptions,
        OptixPipelineLinkOptions const& linkOptions,
        std::span<ProgramGroup const> groups);

    ~Pipeline() noexcept;

    Pipeline(Pipeline&& other) noexcept;

    Pipeline& operator=(Pipeline&& other) noexcept;

    OptixPipeline handle() const noexcept {
        return m_handle;
    }

    void
    setStackSize(uint32_t directTraversal, uint32_t directState, uint32_t continuation, uint32_t maxTraversableDepth);

    void launch(
        ShaderBindingTable const& sbt,
        CUdeviceptr params,
        size_t paramsSize,
        cudaStream_t stream,
        uint32_t width,
        uint32_t height,
        uint32_t depth = 1) const;

private:
    OptixPipeline m_handle{};
};

} // namespace optix

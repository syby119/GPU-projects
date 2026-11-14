#pragma once

#include <optix/device_context.h>
#include <gpu/buffer.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace optix::detail {

class AccelerationStructure {
public:
    AccelerationStructure() = default;

    AccelerationStructure(
        DeviceContext const& context,
        OptixAccelBuildOptions options,
        std::span<OptixBuildInput const> inputs,
        cudaStream_t stream = nullptr);

    AccelerationStructure(AccelerationStructure&& other) noexcept;

    AccelerationStructure& operator=(AccelerationStructure&& other) noexcept;

    OptixTraversableHandle handle() const noexcept {
        return m_handle;
    }

    size_t storageSize() const noexcept {
        return m_storage.sizeBytes();
    }

    // Counts and layout are checked; callers must preserve index connectivity and SBT topology.
    void update(std::span<OptixBuildInput const> inputs, cudaStream_t stream = nullptr);

    void compact(cudaStream_t stream = nullptr);

private:
    OptixDeviceContext m_context{}; // Borrowed; context must outlive this object.
    OptixAccelBuildOptions m_options{};
    gpu::Buffer<std::byte> m_storage;
    OptixTraversableHandle m_handle{};
    size_t m_compactedSize{};
    uint32_t m_inputCount{};
    std::vector<uint64_t> m_topology;
};

} // namespace optix::detail

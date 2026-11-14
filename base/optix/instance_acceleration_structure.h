#pragma once

#include <optix/detail/acceleration_structure.h>

#include <array>
#include <span>
#include <vector>

namespace optix {

// OptixInstance::transform is a row-major 3x4 object-to-world matrix.
OptixInstance makeInstance(
    OptixTraversableHandle traversable,
    std::array<float, 12> const& transform,
    uint32_t instanceId = 0,
    uint32_t sbtOffset = 0,
    uint32_t visibilityMask = 255,
    uint32_t flags = OPTIX_INSTANCE_FLAG_NONE) noexcept;

// Owns the device instance array and IAS. Referenced children (and their descendants) must outlive this IAS.
// Rebuild the IAS if a referenced traversable changes its handle.
class InstanceAccelerationStructure {
public:
    InstanceAccelerationStructure() = default;

    InstanceAccelerationStructure(
        DeviceContext const& context,
        OptixAccelBuildOptions options,
        std::span<OptixInstance const> instances,
        cudaStream_t stream = nullptr);

    InstanceAccelerationStructure(InstanceAccelerationStructure&&) noexcept = default;
    InstanceAccelerationStructure& operator=(InstanceAccelerationStructure&&) noexcept = default;

    OptixTraversableHandle handle() const noexcept {
        return m_accelerationStructure.handle();
    }

    size_t storageSize() const noexcept {
        return m_accelerationStructure.storageSize();
    }

    // Requires ALLOW_UPDATE, the same instance count, and unchanged child handles.
    void update(std::span<OptixInstance const> instances, cudaStream_t stream = nullptr);

    void compact(cudaStream_t stream = nullptr) {
        m_accelerationStructure.compact(stream);
    }

private:
    gpu::Buffer<OptixInstance> m_instances;
    detail::AccelerationStructure m_accelerationStructure;
    std::vector<OptixTraversableHandle> m_childHandles;
};

} // namespace optix

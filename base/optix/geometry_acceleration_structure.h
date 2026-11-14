#pragma once

#include <optix/detail/acceleration_structure.h>

namespace optix {

// Owns a GAS. The device context must outlive it; updates need valid geometry input buffers.
class GeometryAccelerationStructure {
public:
    GeometryAccelerationStructure() = default;

    GeometryAccelerationStructure(
        DeviceContext const& context,
        OptixAccelBuildOptions options,
        std::span<OptixBuildInput const> inputs,
        cudaStream_t stream = nullptr);

    GeometryAccelerationStructure(GeometryAccelerationStructure&&) noexcept = default;

    GeometryAccelerationStructure& operator=(GeometryAccelerationStructure&&) noexcept = default;

    OptixTraversableHandle handle() const noexcept {
        return m_accelerationStructure.handle();
    }

    size_t storageSize() const noexcept {
        return m_accelerationStructure.storageSize();
    }

    void update(std::span<OptixBuildInput const> inputs, cudaStream_t stream = nullptr);

    void compact(cudaStream_t stream = nullptr) {
        m_accelerationStructure.compact(stream);
    }

private:
    detail::AccelerationStructure m_accelerationStructure;
};

} // namespace optix

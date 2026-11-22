#pragma once

#include <optix/device_context.h>

#include <gpu/buffer/buffer.h>

namespace optix {

// Matrix node for an IAS -> StaticTransform -> GAS traversable graph.
// descriptor.transform and descriptor.invTransform must be inverse row-major 3x4 matrices.
// The child traversable and device context must outlive this object.
// If the child handle changes, create a new StaticTransform and rebuild its parent IAS.
class StaticTransform {
public:
    StaticTransform() noexcept;

    StaticTransform(DeviceContext const& context, OptixStaticTransform const& descriptor);

    StaticTransform(StaticTransform const&) = delete;
    StaticTransform& operator=(StaticTransform const&) = delete;

    StaticTransform(StaticTransform&& other) noexcept;
    StaticTransform& operator=(StaticTransform&& other) noexcept;

    OptixTraversableHandle handle() const noexcept {
        return m_handle;
    }

private:
    gpu::Buffer<OptixStaticTransform, 1> m_storage;
    OptixTraversableHandle m_handle{};
};

} // namespace optix

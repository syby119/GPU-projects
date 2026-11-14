#pragma once

#include <optix/device_context.h>
#include <gpu/buffer.h>

#include <cstdint>
#include <span>

namespace optix {

class Denoiser {
public:
    Denoiser() = default;

    Denoiser(DeviceContext const& context, OptixDenoiserModelKind model, OptixDenoiserOptions const& options = {});

    ~Denoiser() noexcept;

    Denoiser(Denoiser&& other) noexcept;

    Denoiser& operator=(Denoiser&& other) noexcept;

    OptixDenoiser handle() const noexcept {
        return m_handle;
    }

    // Allocates state and scratch for the maximum image size. Completes before returning.
    void setup(uint32_t width, uint32_t height, bool tiled = false, cudaStream_t stream = nullptr);

    OptixDenoiserSizes const& sizes() const noexcept {
        return m_sizes;
    }

    // Asynchronous. Use one stream and keep images alive until that stream completes.
    void invoke(
        OptixDenoiserParams const& params,
        OptixDenoiserGuideLayer const& guide,
        std::span<OptixDenoiserLayer const> layers,
        cudaStream_t stream = nullptr,
        uint32_t inputOffsetX = 0,
        uint32_t inputOffsetY = 0) const;

    void computeIntensity(OptixImage2D const& input, CUdeviceptr output, cudaStream_t stream = nullptr) const;

    void computeAverageColor(OptixImage2D const& input, CUdeviceptr output, cudaStream_t stream = nullptr) const;

private:
    OptixDenoiser m_handle{};
    OptixDenoiserSizes m_sizes{};
    gpu::Buffer<std::byte> m_state;
    gpu::Buffer<std::byte> m_scratch;
};

} // namespace optix

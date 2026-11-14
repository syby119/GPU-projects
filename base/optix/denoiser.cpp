#include <optix/denoiser.h>

#include <gpu/utility.h>

#include <algorithm>
#include <utility>

namespace optix {

Denoiser::Denoiser(DeviceContext const& context, OptixDenoiserModelKind model, OptixDenoiserOptions const& options) {
    CHECK_OPTIX(optixDenoiserCreate(context.handle(), model, &options, &m_handle));
}

Denoiser::~Denoiser() noexcept {
    if (m_handle) {
        LOG_OPTIX(optixDenoiserDestroy(m_handle));
    }
}

Denoiser::Denoiser(Denoiser&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_sizes(std::exchange(other.m_sizes, OptixDenoiserSizes{}))
    , m_state(std::move(other.m_state))
    , m_scratch(std::move(other.m_scratch)) {}

Denoiser& Denoiser::operator=(Denoiser&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            LOG_OPTIX(optixDenoiserDestroy(m_handle));
        }
        m_handle = std::exchange(other.m_handle, nullptr);
        m_sizes = std::exchange(other.m_sizes, OptixDenoiserSizes{});
        m_state = std::move(other.m_state);
        m_scratch = std::move(other.m_scratch);
    }
    return *this;
}

void Denoiser::setup(uint32_t width, uint32_t height, bool tiled, cudaStream_t stream) {
    OptixDenoiserSizes sizes{};
    CHECK_OPTIX(optixDenoiserComputeMemoryResources(m_handle, width, height, &sizes));
    m_state.reallocate(sizes.stateSizeInBytes);
    m_scratch.reallocate(
        std::max(
            {tiled ? sizes.withOverlapScratchSizeInBytes : sizes.withoutOverlapScratchSizeInBytes,
             sizes.computeIntensitySizeInBytes,
             sizes.computeAverageColorSizeInBytes}));
    CHECK_OPTIX(optixDenoiserSetup(
        m_handle,
        stream,
        width,
        height,
        address(m_state.data()),
        m_state.sizeBytes(),
        address(m_scratch.data()),
        m_scratch.sizeBytes()));
    CHECK_CUDA(cudaStreamSynchronize(stream));
    m_sizes = sizes;
}

void Denoiser::invoke(
    OptixDenoiserParams const& params,
    OptixDenoiserGuideLayer const& guide,
    std::span<OptixDenoiserLayer const> layers,
    cudaStream_t stream,
    uint32_t inputOffsetX,
    uint32_t inputOffsetY) const {
    CHECK_OPTIX(optixDenoiserInvoke(
        m_handle,
        stream,
        &params,
        address(m_state.data()),
        m_state.sizeBytes(),
        &guide,
        layers.data(),
        static_cast<uint32_t>(layers.size()),
        inputOffsetX,
        inputOffsetY,
        address(m_scratch.data()),
        m_scratch.sizeBytes()));
}

void Denoiser::computeIntensity(OptixImage2D const& input, CUdeviceptr output, cudaStream_t stream) const {
    CHECK_OPTIX(optixDenoiserComputeIntensity(
        m_handle, stream, &input, output, address(m_scratch.data()), m_scratch.sizeBytes()));
}

void Denoiser::computeAverageColor(OptixImage2D const& input, CUdeviceptr output, cudaStream_t stream) const {
    CHECK_OPTIX(optixDenoiserComputeAverageColor(
        m_handle, stream, &input, output, address(m_scratch.data()), m_scratch.sizeBytes()));
}

} // namespace optix

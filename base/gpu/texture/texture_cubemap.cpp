#include <gpu/texture/texture_cubemap.h>

#include <stdexcept>
#include <utility>

namespace gpu {

static size_t pixelBytes(cudaChannelFormatDesc const& desc) noexcept {
    return static_cast<size_t>(desc.x + desc.y + desc.z + desc.w) / 8;
}

TextureCubemap::TextureCubemap(Specification const& specification)
    : m_specification(specification) {
    if (specification.arrayFlags & cudaArrayLayered) {
        throw std::invalid_argument("TextureCubemap cannot use layered array flags");
    }
    CHECK_CUDA(cudaMalloc3DArray(
        &m_array,
        &specification.format,
        make_cudaExtent(specification.size, specification.size, 6),
        specification.arrayFlags | cudaArrayCubemap));

    cudaResourceDesc resource{};
    resource.resType = cudaResourceTypeArray;
    resource.res.array.array = m_array;
    cudaError_t const status = cudaCreateTextureObject(&m_texture, &resource, &specification.texDesc, nullptr);
    if (status != cudaSuccess) {
        LOG_CUDA(cudaFreeArray(std::exchange(m_array, nullptr)));
        checkCudaImpl(status, "cudaCreateTextureObject", __FILE__, __LINE__);
    }
}

TextureCubemap::TextureCubemap(TextureCubemap&& other) noexcept
    : m_specification(other.m_specification)
    , m_array(std::exchange(other.m_array, nullptr))
    , m_texture(std::exchange(other.m_texture, 0)) {}

TextureCubemap::~TextureCubemap() noexcept {
    destroy();
}

TextureCubemap& TextureCubemap::operator=(TextureCubemap&& other) noexcept {
    if (this != &other) {
        destroy();
        m_specification = other.m_specification;
        m_array = std::exchange(other.m_array, nullptr);
        m_texture = std::exchange(other.m_texture, 0);
    }

    return *this;
}

void TextureCubemap::destroy() noexcept {
    if (m_texture) {
        LOG_CUDA(cudaDestroyTextureObject(std::exchange(m_texture, 0)));
    }
    if (m_array) {
        LOG_CUDA(cudaFreeArray(std::exchange(m_array, nullptr)));
    }
}

void TextureCubemap::copyFrom(Face face, void const* data, size_t pitch, cudaMemcpyKind kind) {
    uint32_t const faceIndex = static_cast<uint32_t>(face);
    size_t const rowBytes = size() * pixelBytes(m_specification.format);

    cudaMemcpy3DParms copy{};
    copy.srcPtr = make_cudaPitchedPtr(const_cast<void*>(data), pitch, rowBytes, size());
    copy.dstArray = m_array;
    copy.dstPos = make_cudaPos(0, 0, faceIndex);
    copy.extent = make_cudaExtent(size(), size(), 1);
    copy.kind = kind;
    CHECK_CUDA(cudaMemcpy3D(&copy));
}

void TextureCubemap::copyFromHost(Face face, void const* data, size_t rowPitchBytes) {
    copyFrom(face, data, rowPitchBytes, cudaMemcpyHostToDevice);
}

void TextureCubemap::copyFromDevice(Face face, void const* data, size_t rowPitchBytes) {
    copyFrom(face, data, rowPitchBytes, cudaMemcpyDeviceToDevice);
}

} // namespace gpu

#include <optix/detail/acceleration_structure.h>

#include <gpu/utility.h>

#include <stdexcept>
#include <utility>

namespace optix::detail {

static std::vector<uint64_t> topology(std::span<OptixBuildInput const> inputs) {
    std::vector<uint64_t> result;
    result.reserve(inputs.size() * 3);
    for (auto const& input : inputs) {
        result.push_back(input.type);
        switch (input.type) {
        case OPTIX_BUILD_INPUT_TYPE_TRIANGLES:
            result.push_back(input.triangleArray.numVertices);
            result.push_back(input.triangleArray.numIndexTriplets);
            result.push_back(input.triangleArray.vertexFormat);
            result.push_back(input.triangleArray.indexFormat);
            result.push_back(input.triangleArray.numSbtRecords);
            break;
        case OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES:
            result.push_back(input.customPrimitiveArray.numPrimitives);
            result.push_back(input.customPrimitiveArray.numSbtRecords);
            break;
        case OPTIX_BUILD_INPUT_TYPE_INSTANCES:
        case OPTIX_BUILD_INPUT_TYPE_INSTANCE_POINTERS:
            result.push_back(input.instanceArray.numInstances);
            result.push_back(0);
            break;
        case OPTIX_BUILD_INPUT_TYPE_CURVES:
            result.push_back(input.curveArray.numPrimitives);
            result.push_back(input.curveArray.numVertices);
            result.push_back(input.curveArray.curveType);
            break;
        case OPTIX_BUILD_INPUT_TYPE_SPHERES:
            result.push_back(input.sphereArray.numVertices);
            result.push_back(input.sphereArray.numSbtRecords);
            break;
        default:
            throw std::invalid_argument(
                "Acceleration structure update topology validation does not support this build input type");
        }
    }
    return result;
}

AccelerationStructure::AccelerationStructure(
    DeviceContext const& context,
    OptixAccelBuildOptions options,
    std::span<OptixBuildInput const> inputs,
    cudaStream_t stream)
    : m_context(context.handle())
    , m_options(options)
    , m_inputCount(static_cast<uint32_t>(inputs.size())) {
    if (inputs.empty()) {
        throw std::invalid_argument("AccelerationStructure requires build inputs");
    }
    if (options.buildFlags & OPTIX_BUILD_FLAG_ALLOW_UPDATE) {
        m_topology = topology(inputs);
    }
    m_options.operation = OPTIX_BUILD_OPERATION_BUILD;
    OptixAccelBufferSizes sizes{};
    CHECK_OPTIX(optixAccelComputeMemoryUsage(m_context, &m_options, inputs.data(), m_inputCount, &sizes));
    gpu::Buffer<std::byte> scratch(sizes.tempSizeInBytes);
    m_storage.reallocate(sizes.outputSizeInBytes);
    gpu::Buffer<size_t, 1> compacted;
    OptixAccelEmitDesc emit{address(compacted.data()), OPTIX_PROPERTY_TYPE_COMPACTED_SIZE};
    bool const allowCompact = (m_options.buildFlags & OPTIX_BUILD_FLAG_ALLOW_COMPACTION) != 0;
    CHECK_OPTIX(optixAccelBuild(
        m_context,
        stream,
        &m_options,
        inputs.data(),
        m_inputCount,
        address(scratch.data()),
        scratch.sizeBytes(),
        address(m_storage.data()),
        m_storage.sizeBytes(),
        &m_handle,
        allowCompact ? &emit : nullptr,
        allowCompact ? 1 : 0));
    CHECK_CUDA(cudaStreamSynchronize(stream));
    if (allowCompact) {
        compacted.copyToHost(&m_compactedSize);
    }
}

AccelerationStructure::AccelerationStructure(AccelerationStructure&& other) noexcept
    : m_context(std::exchange(other.m_context, nullptr))
    , m_options(other.m_options)
    , m_storage(std::move(other.m_storage))
    , m_handle(std::exchange(other.m_handle, 0))
    , m_compactedSize(std::exchange(other.m_compactedSize, 0))
    , m_inputCount(std::exchange(other.m_inputCount, 0))
    , m_topology(std::move(other.m_topology)) {}

AccelerationStructure& AccelerationStructure::operator=(AccelerationStructure&& other) noexcept {
    if (this != &other) {
        m_context = std::exchange(other.m_context, nullptr);
        m_options = other.m_options;
        m_storage = std::move(other.m_storage);
        m_handle = std::exchange(other.m_handle, 0);
        m_compactedSize = std::exchange(other.m_compactedSize, 0);
        m_inputCount = std::exchange(other.m_inputCount, 0);
        m_topology = std::move(other.m_topology);
    }

    return *this;
}

void AccelerationStructure::update(std::span<OptixBuildInput const> inputs, cudaStream_t stream) {
    if (!(m_options.buildFlags & OPTIX_BUILD_FLAG_ALLOW_UPDATE) || !m_handle) {
        throw std::logic_error("Acceleration structure was not built with ALLOW_UPDATE");
    }
    if (inputs.size() != m_inputCount || topology(inputs) != m_topology) {
        throw std::invalid_argument("Acceleration structure topology changed");
    }
    OptixAccelBuildOptions options = m_options;
    options.operation = OPTIX_BUILD_OPERATION_UPDATE;
    OptixAccelBufferSizes sizes{};
    CHECK_OPTIX(optixAccelComputeMemoryUsage(m_context, &options, inputs.data(), m_inputCount, &sizes));
    gpu::Buffer<std::byte> scratch(sizes.tempUpdateSizeInBytes);
    CHECK_OPTIX(optixAccelBuild(
        m_context,
        stream,
        &options,
        inputs.data(),
        m_inputCount,
        address(scratch.data()),
        scratch.sizeBytes(),
        address(m_storage.data()),
        m_storage.sizeBytes(),
        &m_handle,
        nullptr,
        0));
    CHECK_CUDA(cudaStreamSynchronize(stream));
}

void AccelerationStructure::compact(cudaStream_t stream) {
    if (!(m_options.buildFlags & OPTIX_BUILD_FLAG_ALLOW_COMPACTION) || !m_handle) {
        throw std::logic_error("Acceleration structure was not built with ALLOW_COMPACTION");
    }
    if (m_compactedSize == 0 || m_compactedSize >= m_storage.sizeBytes()) {
        return;
    }
    gpu::Buffer<std::byte> compacted(m_compactedSize);
    OptixTraversableHandle handle{};
    CHECK_OPTIX(
        optixAccelCompact(m_context, stream, m_handle, address(compacted.data()), compacted.sizeBytes(), &handle));
    CHECK_CUDA(cudaStreamSynchronize(stream));
    m_storage = std::move(compacted);
    m_handle = handle;
}

} // namespace optix::detail

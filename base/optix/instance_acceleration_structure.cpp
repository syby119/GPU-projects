#include <optix/instance_acceleration_structure.h>

#include <algorithm>
#include <stdexcept>

namespace optix {

OptixInstance makeInstance(
    OptixTraversableHandle traversable,
    std::array<float, 12> const& transform,
    uint32_t instanceId,
    uint32_t sbtOffset,
    uint32_t visibilityMask,
    uint32_t flags) noexcept {
    OptixInstance instance{};
    std::copy(transform.begin(), transform.end(), instance.transform);
    instance.instanceId = instanceId;
    instance.sbtOffset = sbtOffset;
    instance.visibilityMask = visibilityMask;
    instance.flags = flags;
    instance.traversableHandle = traversable;
    return instance;
}

static OptixBuildInput buildInput(gpu::Buffer<OptixInstance> const& instances) {
    OptixBuildInput input{};
    input.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    input.instanceArray.instances = address(instances.data());
    input.instanceArray.numInstances = static_cast<uint32_t>(instances.size());
    return input;
}

InstanceAccelerationStructure::InstanceAccelerationStructure(
    DeviceContext const& context,
    OptixAccelBuildOptions options,
    std::span<OptixInstance const> instances,
    cudaStream_t stream)
    : m_instances(instances.size()) {
    m_instances.copyFromHost(instances);
    OptixBuildInput const input = buildInput(m_instances);
    m_accelerationStructure = detail::AccelerationStructure(context, options, {&input, 1}, stream);
    m_childHandles.reserve(instances.size());
    for (OptixInstance const& instance : instances) {
        m_childHandles.push_back(instance.traversableHandle);
    }
}

void InstanceAccelerationStructure::update(std::span<OptixInstance const> instances, cudaStream_t stream) {
    if (instances.size() != m_instances.size()) {
        throw std::invalid_argument("IAS update requires the same instance count");
    }
    for (size_t i = 0; i < instances.size(); ++i) {
        if (instances[i].traversableHandle != m_childHandles[i]) {
            throw std::invalid_argument("IAS update requires unchanged child handles; rebuild the IAS instead");
        }
    }
    m_instances.copyFromHost(instances);
    OptixBuildInput const input = buildInput(m_instances);
    m_accelerationStructure.update({&input, 1}, stream);
}

} // namespace optix

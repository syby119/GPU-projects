#include <optix/geometry_acceleration_structure.h>

#include <stdexcept>

namespace optix {

static std::span<OptixBuildInput const> geometryInputs(std::span<OptixBuildInput const> inputs) {
    for (OptixBuildInput const& input : inputs) {
        if (input.type == OPTIX_BUILD_INPUT_TYPE_INSTANCES || input.type == OPTIX_BUILD_INPUT_TYPE_INSTANCE_POINTERS) {
            throw std::invalid_argument("GeometryAccelerationStructure requires geometry build inputs");
        }
    }
    return inputs;
}

GeometryAccelerationStructure::GeometryAccelerationStructure(
    DeviceContext const& context,
    OptixAccelBuildOptions options,
    std::span<OptixBuildInput const> inputs,
    cudaStream_t stream)
    : m_accelerationStructure(context, options, geometryInputs(inputs), stream) {}

void GeometryAccelerationStructure::update(std::span<OptixBuildInput const> inputs, cudaStream_t stream) {
    m_accelerationStructure.update(geometryInputs(inputs), stream);
}

} // namespace optix

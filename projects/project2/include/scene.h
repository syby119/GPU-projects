#pragma once

#include <launch_params.h>

#include <gpu/buffer/buffer.h>
#include <gpu/texture/texture_cubemap.h>
#include <optix/device_context.h>
#include <optix/geometry_acceleration_structure.h>
#include <optix/instance_acceleration_structure.h>
#include <optix/static_transform.h>

#include <array>
#include <filesystem>
#include <memory>

class Scene {
public:
    Scene(optix::DeviceContext const& context, std::filesystem::path const& assetRootDir);

    OptixTraversableHandle traversable() const noexcept {
        return m_acceleration.handle();
    }

    cudaTextureObject_t skyboxTexture() const noexcept {
        return m_skybox->texture();
    }

    float3 const* triangleNormals() const noexcept {
        return m_normals.data();
    }

    Material const* sphereMaterials() const noexcept {
        return m_sphereMaterials.data();
    }

    std::array<Material, 3> const& modelMaterials() const noexcept {
        return m_modelMaterials;
    }

private:
    // Non-owning; the application keeps the context alive longer than the scene.
    optix::DeviceContext const& m_context;

    // Shared model geometry and the material assigned to each model instance.
    gpu::Buffer<float3> m_vertices;
    gpu::Buffer<float3> m_normals;
    std::array<Material, 3> m_modelMaterials{};
    optix::GeometryAccelerationStructure m_modelGas;

    // Sphere geometry and per-sphere materials.
    gpu::Buffer<float3> m_sphereCenters;
    gpu::Buffer<float> m_sphereRadii;
    gpu::Buffer<Material> m_sphereMaterials;
    optix::GeometryAccelerationStructure m_sphereGas;

    // The IAS references the model transforms and sphere GAS, so it is declared last
    // among the acceleration structures and destroyed first.
    std::array<optix::StaticTransform, 3> m_modelNodes;
    optix::InstanceAccelerationStructure m_acceleration;

    std::unique_ptr<gpu::TextureCubemap> m_skybox;

private:
    void createGeometry(std::filesystem::path const& assetRootDir);

    void createSkybox(std::filesystem::path const& assetRootDir);
};

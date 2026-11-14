#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "graphics/rhi_resource.h"
#include "graphics/shader_module.h"

namespace gfx {
class GLProgram : public RHIResource {
public:
    // OpenGLShadingLanguageTypeTokens
    // @ref https://registry.khronos.org/OpenGL/specs/gl/glspec45.core.pdf
    enum class VarType {
        Unknown,
        Bool,
        BVec2,
        BVec3,
        BVec4,
        Int,
        IVec2,
        IVec3,
        IVec4,
        UInt,
        UVec2,
        UVec3,
        UVec4,
        Float,
        Vec2,
        Vec3,
        Vec4,
        Double,
        DVec2,
        DVec3,
        DVec4,
        Mat2x2,
        Mat2x3,
        Mat2x4,
        Mat3x2,
        Mat3x3,
        Mat3x4,
        Mat4x2,
        Mat4x3,
        Mat4x4,
        DMat2x2,
        DMat2x3,
        DMat2x4,
        DMat3x2,
        DMat3x3,
        DMat3x4,
        DMat4x2,
        DMat4x3,
        DMat4x4,
        Sampler1D,
        Sampler2D,
        Sampler3D,
        SamplerCubemap,
        Sampler1DShadow,
        Sampler2DShadow,
        Sampler1DArray,
        Sampler2DArray,
        SamplerCubeArray,
        Sampler1DArrayShadow,
        Sampler2DArrayShadow,
        Sampler2DMultiSample,
        Sampler2DMultiSampleArray,
        SamplerCubemapShadow,
        SamplerCubemapArrayShadow,
        SamplerBuffer,
        Sampler2DRect,
        Sampler2DRectShadow,
        ISampler1D,
        ISampler2D,
        ISampler3D,
        ISamplerCubemap,
        ISampler1DArray,
        ISampler2DArray,
        ISamplerCubemapArray,
        ISampler2DMultiSample,
        ISampler2DMultiSampleArray,
        ISamplerBuffer,
        ISampler2DRect,
        USampler1D,
        USampler2D,
        USampler3D,
        USamplerCubemap,
        USampler1DArray,
        USampler2DArray,
        USamplerCubemapArray,
        USampler2DMultiSample,
        USampler2DMultiSampleArray,
        USamplerBuffer,
        USampler2DRect,
        Image1D,
        Image2D,
        Image3D,
        Image2DRect,
        ImageCubemap,
        ImageBuffer,
        Image1DArray,
        Image2DArray,
        ImageCubemapArray,
        Image2DMultiSample,
        Image2DMultiSampleArray,
        IImage1D,
        IImage2D,
        IImage3D,
        IImage2DRect,
        IImageCubemap,
        IImageBuffer,
        IImage1DArray,
        IImage2DArray,
        IImageCubemapArray,
        IImage2DMultiSample,
        IImage2DMultiSampleArray,
        UImage1D,
        UImage2D,
        UImage3D,
        UImage2DRect,
        UImageCubemap,
        UImageBuffer,
        UImage1DArray,
        UImage2DArray,
        UImageCubemapArray,
        UImage2DMultiSample,
        UImage2DMultiSampleArray,
        AtomicCounter,
        // Not in OpenGL
        AccelerationStructure,
        RayQuery
    };

public:
    struct UniformVarInfo {
        int location;
        VarType type;
    };

    struct TextureInfo {
        int binding;
        VarType type;
    };

    struct UniformBufferInfo {
        int binding;
        uint32_t size;
    };

    struct StorageBufferInfo {
        int binding;
    };

    struct StorageImageInfo {
        int binding;
        VarType type;
    };

    struct AtomicCounterInfo {
        int binding;
        int offset;
    };

    struct TransformFeedbackVaryingInfo {
        VarType type;
        uint32_t arraySize; // GL_ARRAY_SIZE; one for non-array variables
        int offset;         // Byte offset within one captured vertex
    };

    using TransformFeedbackVaryingInfoMap = std::map<std::string, TransformFeedbackVaryingInfo, std::less<>>;

    struct TransformFeedbackBufferInfo {
        int stride; // Bytes between consecutive captured vertices
        TransformFeedbackVaryingInfoMap varyings;
    };

    using UniformVarInfoMap = std::map<std::string, UniformVarInfo, std::less<>>;

    using TextureInfoMap = std::map<std::string, TextureInfo, std::less<>>;

    using UniformBufferInfoMap = std::map<std::string, UniformBufferInfo, std::less<>>;

    using StorageBufferInfoMap = std::map<std::string, StorageBufferInfo, std::less<>>;

    using StorageImageInfoMap = std::map<std::string, StorageImageInfo, std::less<>>;

    using AtomicCounterInfoMap = std::map<std::string, AtomicCounterInfo, std::less<>>;

    using TransformFeedbackBufferInfoMap = std::map<int, TransformFeedbackBufferInfo>;

public:
    GLProgram(std::vector<ShaderModule> const& shaderModules);

    GLProgram(
        std::vector<ShaderModule> const& shaderModules,
        std::map<ShaderModule::Stage, UniformVarInfoMap> const& uniformVarInfos,
        std::map<ShaderModule::Stage, TextureInfoMap> const& textureInfos,
        std::map<ShaderModule::Stage, UniformBufferInfoMap> const& uniformBufferInfos,
        std::map<ShaderModule::Stage, StorageBufferInfoMap> const& storageBufferInfos,
        std::map<ShaderModule::Stage, StorageImageInfoMap> const& storageImageInfos,
        std::map<ShaderModule::Stage, AtomicCounterInfoMap> const& atomicCounterInfos,
        TransformFeedbackBufferInfoMap const& transformFeedbackBufferInfos = {});

    GLProgram(GLProgram&& rhs) noexcept = default;

    ~GLProgram();

    GLProgram& operator=(GLProgram&& rhs) noexcept;

    void use() const;

    static void unuse();

    int getUniformVarLocation(std::string_view name) const;

    int getTextureBinding(std::string_view name) const;

    void setUniform(int location, bool const& value) const;

    void setUniform(int location, glm::bvec2 const& value) const;

    void setUniform(int location, glm::bvec3 const& value) const;

    void setUniform(int location, glm::bvec4 const& value) const;

    void setUniform(int location, int const& value) const;

    void setUniform(int location, glm::ivec2 const& value) const;

    void setUniform(int location, glm::ivec3 const& value) const;

    void setUniform(int location, glm::ivec4 const& value) const;

    void setUniform(int location, uint32_t const& value) const;

    void setUniform(int location, glm::uvec2 const& value) const;

    void setUniform(int location, glm::uvec3 const& value) const;

    void setUniform(int location, glm::uvec4 const& value) const;

    void setUniform(int location, float const& value) const;

    void setUniform(int location, glm::vec2 const& value) const;

    void setUniform(int location, glm::vec3 const& value) const;

    void setUniform(int location, glm::vec4 const& value) const;

    void setUniform(int location, double const& value) const;

    void setUniform(int location, glm::dvec2 const& value) const;

    void setUniform(int location, glm::dvec3 const& value) const;

    void setUniform(int location, glm::dvec4 const& value) const;

    void setUniform(int location, glm::mat2x2 const& value) const;

    void setUniform(int location, glm::mat2x3 const& value) const;

    void setUniform(int location, glm::mat2x4 const& value) const;

    void setUniform(int location, glm::mat3x2 const& value) const;

    void setUniform(int location, glm::mat3x3 const& value) const;

    void setUniform(int location, glm::mat3x4 const& value) const;

    void setUniform(int location, glm::mat4x2 const& value) const;

    void setUniform(int location, glm::mat4x3 const& value) const;

    void setUniform(int location, glm::mat4x4 const& value) const;

    void setUniform(int location, glm::dmat2x2 const& value) const;

    void setUniform(int location, glm::dmat2x3 const& value) const;

    void setUniform(int location, glm::dmat2x4 const& value) const;

    void setUniform(int location, glm::dmat3x2 const& value) const;

    void setUniform(int location, glm::dmat3x3 const& value) const;

    void setUniform(int location, glm::dmat3x4 const& value) const;

    void setUniform(int location, glm::dmat4x2 const& value) const;

    void setUniform(int location, glm::dmat4x3 const& value) const;

    void setUniform(int location, glm::dmat4x4 const& value) const;

    // This API is not recommended to use,
    // because it will query the uniform variable location every time, which is inefficient.
    template <typename T>
    void setUniform(std::string_view name, T const& value) const {
        int location = getUniformVarLocation(name);
        if (location != -1) {
            setUniform(location, value);
        }
    }

    void printResourceInfos() const;

    auto const& getUniformVarInfos() const noexcept {
        return m_uniformVarInfos;
    }

    auto const& getTextureInfos() const noexcept {
        return m_textureInfos;
    }

    auto const& getUniformBufferInfos() const noexcept {
        return m_uniformBufferInfos;
    }

    auto const& getStorageBufferInfos() const noexcept {
        return m_storageBufferInfos;
    }

    auto const& getStorageImageInfos() const noexcept {
        return m_storageImageInfos;
    }

    auto const& getAtomicCounterInfos() const noexcept {
        return m_atomicCounterInfos;
    }

    auto const& getTransformFeedbackBufferInfos() const noexcept {
        return m_transformFeedbackBufferInfos;
    }

private:
    UniformVarInfoMap m_uniformVarInfos;

    TextureInfoMap m_textureInfos;

    UniformBufferInfoMap m_uniformBufferInfos;

    StorageBufferInfoMap m_storageBufferInfos;

    StorageImageInfoMap m_storageImageInfos;

    AtomicCounterInfoMap m_atomicCounterInfos;

    TransformFeedbackBufferInfoMap m_transformFeedbackBufferInfos;

private:
    void attach(ShaderModule const& shaderModule);

    void detach(ShaderModule const& shaderModule);

    void link();

    void queryResourceMetaInfos();

    void queryGenericUniforms();

    void queryUniformBlocks();

    void queryShaderStorageBuffers();

    void queryAtomicCounters();

    void queryTransformFeedbackInfo();

    void printUniformVarInfos() const;

    void printTextureInfos() const;

    void printUniformBlockInfos() const;

    void printStorageBufferInfos() const;

    void printStorageImageInfos() const;

    void printAtomicCounterInfos() const;

    void printTransformFeedbackInfo() const;

    static bool isTexture(VarType type) noexcept;

    static bool isImage(VarType type) noexcept;
};
} // namespace gfx

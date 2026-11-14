#include "graphics/gl_program.h"

#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "graphics/gl_utility.h"

namespace gfx {

static std::string_view getVarTypeName(GLProgram::VarType type) noexcept {
    switch (type) {
    case GLProgram::VarType::Unknown:
        return "Unknown";
    case GLProgram::VarType::Bool:
        return "Bool";
    case GLProgram::VarType::BVec2:
        return "BVec2";
    case GLProgram::VarType::BVec3:
        return "BVec3";
    case GLProgram::VarType::BVec4:
        return "BVec4";
    case GLProgram::VarType::Int:
        return "Int";
    case GLProgram::VarType::IVec2:
        return "IVec2";
    case GLProgram::VarType::IVec3:
        return "IVec3";
    case GLProgram::VarType::IVec4:
        return "IVec4";
    case GLProgram::VarType::UInt:
        return "UInt";
    case GLProgram::VarType::UVec2:
        return "UVec2";
    case GLProgram::VarType::UVec3:
        return "UVec3";
    case GLProgram::VarType::UVec4:
        return "UVec4";
    case GLProgram::VarType::Float:
        return "Float";
    case GLProgram::VarType::Vec2:
        return "Vec2";
    case GLProgram::VarType::Vec3:
        return "Vec3";
    case GLProgram::VarType::Vec4:
        return "Vec4";
    case GLProgram::VarType::Double:
        return "Double";
    case GLProgram::VarType::DVec2:
        return "DVec2";
    case GLProgram::VarType::DVec3:
        return "DVec3";
    case GLProgram::VarType::DVec4:
        return "DVec4";
    case GLProgram::VarType::Mat2x2:
        return "Mat2x2";
    case GLProgram::VarType::Mat2x3:
        return "Mat2x3";
    case GLProgram::VarType::Mat2x4:
        return "Mat2x4";
    case GLProgram::VarType::Mat3x2:
        return "Mat3x2";
    case GLProgram::VarType::Mat3x3:
        return "Mat3x3";
    case GLProgram::VarType::Mat3x4:
        return "Mat3x4";
    case GLProgram::VarType::Mat4x2:
        return "Mat4x2";
    case GLProgram::VarType::Mat4x3:
        return "Mat4x3";
    case GLProgram::VarType::Mat4x4:
        return "Mat4x4";
    case GLProgram::VarType::DMat2x2:
        return "DMat2x2";
    case GLProgram::VarType::DMat2x3:
        return "DMat2x3";
    case GLProgram::VarType::DMat2x4:
        return "DMat2x4";
    case GLProgram::VarType::DMat3x2:
        return "DMat3x2";
    case GLProgram::VarType::DMat3x3:
        return "DMat3x3";
    case GLProgram::VarType::DMat3x4:
        return "DMat3x4";
    case GLProgram::VarType::DMat4x2:
        return "DMat4x2";
    case GLProgram::VarType::DMat4x3:
        return "DMat4x3";
    case GLProgram::VarType::DMat4x4:
        return "DMat4x4";
    case GLProgram::VarType::Sampler1D:
        return "Sampler1D";
    case GLProgram::VarType::Sampler2D:
        return "Sampler2D";
    case GLProgram::VarType::Sampler3D:
        return "Sampler3D";
    case GLProgram::VarType::SamplerCubemap:
        return "SamplerCubemap";
    case GLProgram::VarType::Sampler1DShadow:
        return "Sampler1DShadow";
    case GLProgram::VarType::Sampler2DShadow:
        return "Sampler2DShadow";
    case GLProgram::VarType::Sampler1DArray:
        return "Sampler1DArray";
    case GLProgram::VarType::Sampler2DArray:
        return "Sampler2DArray";
    case GLProgram::VarType::SamplerCubeArray:
        return "SamplerCubeArray";
    case GLProgram::VarType::Sampler1DArrayShadow:
        return "Sampler1DArrayShadow";
    case GLProgram::VarType::Sampler2DArrayShadow:
        return "Sampler2DArrayShadow";
    case GLProgram::VarType::Sampler2DMultiSample:
        return "Sampler2DMultiSample";
    case GLProgram::VarType::Sampler2DMultiSampleArray:
        return "Sampler2DMultiSampleArray";
    case GLProgram::VarType::SamplerCubemapShadow:
        return "SamplerCubemapShadow";
    case GLProgram::VarType::SamplerCubemapArrayShadow:
        return "SamplerCubemapArrayShadow";
    case GLProgram::VarType::SamplerBuffer:
        return "SamplerBuffer";
    case GLProgram::VarType::Sampler2DRect:
        return "Sampler2DRect";
    case GLProgram::VarType::Sampler2DRectShadow:
        return "Sampler2DRectShadow";
    case GLProgram::VarType::ISampler1D:
        return "ISampler1D";
    case GLProgram::VarType::ISampler2D:
        return "ISampler2D";
    case GLProgram::VarType::ISampler3D:
        return "ISampler3D";
    case GLProgram::VarType::ISamplerCubemap:
        return "ISamplerCubemap";
    case GLProgram::VarType::ISampler1DArray:
        return "ISampler1DArray";
    case GLProgram::VarType::ISampler2DArray:
        return "ISampler2DArray";
    case GLProgram::VarType::ISamplerCubemapArray:
        return "ISamplerCubemapArray";
    case GLProgram::VarType::ISampler2DMultiSample:
        return "ISampler2DMultiSample";
    case GLProgram::VarType::ISampler2DMultiSampleArray:
        return "ISampler2DMultiSampleArray";
    case GLProgram::VarType::ISamplerBuffer:
        return "ISamplerBuffer";
    case GLProgram::VarType::ISampler2DRect:
        return "ISampler2DRect";
    case GLProgram::VarType::USampler1D:
        return "USampler1D";
    case GLProgram::VarType::USampler2D:
        return "USampler2D";
    case GLProgram::VarType::USampler3D:
        return "USampler3D";
    case GLProgram::VarType::USamplerCubemap:
        return "USamplerCubemap";
    case GLProgram::VarType::USampler1DArray:
        return "USampler1DArray";
    case GLProgram::VarType::USampler2DArray:
        return "USampler2DArray";
    case GLProgram::VarType::USamplerCubemapArray:
        return "USamplerCubemapArray";
    case GLProgram::VarType::USampler2DMultiSample:
        return "USampler2DMultiSample";
    case GLProgram::VarType::USampler2DMultiSampleArray:
        return "USampler2DMultiSampleArray";
    case GLProgram::VarType::USamplerBuffer:
        return "USamplerBuffer";
    case GLProgram::VarType::USampler2DRect:
        return "USampler2DRect";
    case GLProgram::VarType::Image1D:
        return "Image1D";
    case GLProgram::VarType::Image2D:
        return "Image2D";
    case GLProgram::VarType::Image3D:
        return "Image3D";
    case GLProgram::VarType::Image2DRect:
        return "Image2DRect";
    case GLProgram::VarType::ImageCubemap:
        return "ImageCubemap";
    case GLProgram::VarType::ImageBuffer:
        return "ImageBuffer";
    case GLProgram::VarType::Image1DArray:
        return "Image1DArray";
    case GLProgram::VarType::Image2DArray:
        return "Image2DArray";
    case GLProgram::VarType::ImageCubemapArray:
        return "ImageCubemapArray";
    case GLProgram::VarType::Image2DMultiSample:
        return "Image2DMultiSample";
    case GLProgram::VarType::Image2DMultiSampleArray:
        return "Image2DMultiSampleArray";
    case GLProgram::VarType::IImage1D:
        return "IImage1D";
    case GLProgram::VarType::IImage2D:
        return "IImage2D";
    case GLProgram::VarType::IImage3D:
        return "IImage3D";
    case GLProgram::VarType::IImage2DRect:
        return "IImage2DRect";
    case GLProgram::VarType::IImageCubemap:
        return "IImageCubemap";
    case GLProgram::VarType::IImageBuffer:
        return "IImageBuffer";
    case GLProgram::VarType::IImage1DArray:
        return "IImage1DArray";
    case GLProgram::VarType::IImage2DArray:
        return "IImage2DArray";
    case GLProgram::VarType::IImageCubemapArray:
        return "IImageCubemapArray";
    case GLProgram::VarType::IImage2DMultiSample:
        return "IImage2DMultiSample";
    case GLProgram::VarType::IImage2DMultiSampleArray:
        return "IImage2DMultiSampleArray";
    case GLProgram::VarType::UImage1D:
        return "UImage1D";
    case GLProgram::VarType::UImage2D:
        return "UImage2D";
    case GLProgram::VarType::UImage3D:
        return "UImage3D";
    case GLProgram::VarType::UImage2DRect:
        return "UImage2DRect";
    case GLProgram::VarType::UImageCubemap:
        return "UImageCubemap";
    case GLProgram::VarType::UImageBuffer:
        return "UImageBuffer";
    case GLProgram::VarType::UImage1DArray:
        return "UImage1DArray";
    case GLProgram::VarType::UImage2DArray:
        return "UImage2DArray";
    case GLProgram::VarType::UImageCubemapArray:
        return "UImageCubemapArray";
    case GLProgram::VarType::UImage2DMultiSample:
        return "UImage2DMultiSample";
    case GLProgram::VarType::UImage2DMultiSampleArray:
        return "UImage2DMultiSampleArray";
    case GLProgram::VarType::AtomicCounter:
        return "AtomicCounter";
    case GLProgram::VarType::AccelerationStructure:
        return "AccelerationStructure";
    case GLProgram::VarType::RayQuery:
        return "RayQuery";
    }

    return "Unknown";
}

static GLProgram::VarType getVarType(GLint type) {
    switch (type) {
    default:
        break;
    case GL_BOOL:
        return GLProgram::VarType::Bool;
    case GL_BOOL_VEC2:
        return GLProgram::VarType::BVec2;
    case GL_BOOL_VEC3:
        return GLProgram::VarType::BVec3;
    case GL_BOOL_VEC4:
        return GLProgram::VarType::BVec4;
    case GL_INT:
        return GLProgram::VarType::Int;
    case GL_INT_VEC2:
        return GLProgram::VarType::IVec2;
    case GL_INT_VEC3:
        return GLProgram::VarType::IVec3;
    case GL_INT_VEC4:
        return GLProgram::VarType::IVec4;
    case GL_UNSIGNED_INT:
        return GLProgram::VarType::UInt;
    case GL_UNSIGNED_INT_VEC2:
        return GLProgram::VarType::UVec2;
    case GL_UNSIGNED_INT_VEC3:
        return GLProgram::VarType::UVec3;
    case GL_UNSIGNED_INT_VEC4:
        return GLProgram::VarType::UVec4;
    case GL_FLOAT:
        return GLProgram::VarType::Float;
    case GL_FLOAT_VEC2:
        return GLProgram::VarType::Vec2;
    case GL_FLOAT_VEC3:
        return GLProgram::VarType::Vec3;
    case GL_FLOAT_VEC4:
        return GLProgram::VarType::Vec4;
    case GL_DOUBLE:
        return GLProgram::VarType::Double;
    case GL_DOUBLE_VEC2:
        return GLProgram::VarType::DVec2;
    case GL_DOUBLE_VEC3:
        return GLProgram::VarType::DVec3;
    case GL_DOUBLE_VEC4:
        return GLProgram::VarType::DVec4;
    case GL_FLOAT_MAT2:
        return GLProgram::VarType::Mat2x2;
    case GL_FLOAT_MAT2x3:
        return GLProgram::VarType::Mat2x3;
    case GL_FLOAT_MAT2x4:
        return GLProgram::VarType::Mat2x4;
    case GL_FLOAT_MAT3x2:
        return GLProgram::VarType::Mat3x2;
    case GL_FLOAT_MAT3:
        return GLProgram::VarType::Mat3x3;
    case GL_FLOAT_MAT3x4:
        return GLProgram::VarType::Mat3x4;
    case GL_FLOAT_MAT4x2:
        return GLProgram::VarType::Mat4x2;
    case GL_FLOAT_MAT4x3:
        return GLProgram::VarType::Mat4x3;
    case GL_FLOAT_MAT4:
        return GLProgram::VarType::Mat4x4;
    case GL_DOUBLE_MAT2:
        return GLProgram::VarType::DMat2x2;
    case GL_DOUBLE_MAT2x3:
        return GLProgram::VarType::DMat2x3;
    case GL_DOUBLE_MAT2x4:
        return GLProgram::VarType::DMat2x4;
    case GL_DOUBLE_MAT3x2:
        return GLProgram::VarType::DMat3x2;
    case GL_DOUBLE_MAT3:
        return GLProgram::VarType::DMat3x3;
    case GL_DOUBLE_MAT3x4:
        return GLProgram::VarType::DMat3x4;
    case GL_DOUBLE_MAT4x2:
        return GLProgram::VarType::DMat4x2;
    case GL_DOUBLE_MAT4x3:
        return GLProgram::VarType::DMat4x3;
    case GL_DOUBLE_MAT4:
        return GLProgram::VarType::DMat4x4;
    case GL_SAMPLER_1D:
        return GLProgram::VarType::Sampler1D;
    case GL_SAMPLER_2D:
        return GLProgram::VarType::Sampler2D;
    case GL_SAMPLER_3D:
        return GLProgram::VarType::Sampler3D;
    case GL_SAMPLER_CUBE:
        return GLProgram::VarType::SamplerCubemap;
    case GL_SAMPLER_1D_ARRAY:
        return GLProgram::VarType::Sampler1DArray;
    case GL_SAMPLER_2D_ARRAY:
        return GLProgram::VarType::Sampler2DArray;
    case GL_SAMPLER_CUBE_MAP_ARRAY:
        return GLProgram::VarType::SamplerCubeArray;
    case GL_SAMPLER_1D_ARRAY_SHADOW:
        return GLProgram::VarType::Sampler1DArrayShadow;
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        return GLProgram::VarType::Sampler2DArrayShadow;
    case GL_SAMPLER_2D_MULTISAMPLE:
        return GLProgram::VarType::Sampler2DMultiSample;
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::Sampler2DMultiSampleArray;
    case GL_SAMPLER_CUBE_SHADOW:
        return GLProgram::VarType::SamplerCubemapShadow;
    case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
        return GLProgram::VarType::SamplerCubemapArrayShadow;
    case GL_SAMPLER_BUFFER:
        return GLProgram::VarType::SamplerBuffer;
    case GL_SAMPLER_2D_RECT:
        return GLProgram::VarType::Sampler2DRect;
    case GL_SAMPLER_2D_RECT_SHADOW:
        return GLProgram::VarType::Sampler2DRectShadow;
    case GL_INT_SAMPLER_1D:
        return GLProgram::VarType::ISampler1D;
    case GL_INT_SAMPLER_2D:
        return GLProgram::VarType::ISampler2D;
    case GL_INT_SAMPLER_3D:
        return GLProgram::VarType::ISampler3D;
    case GL_INT_SAMPLER_CUBE:
        return GLProgram::VarType::ISamplerCubemap;
    case GL_INT_SAMPLER_1D_ARRAY:
        return GLProgram::VarType::ISampler1DArray;
    case GL_INT_SAMPLER_2D_ARRAY:
        return GLProgram::VarType::ISampler2DArray;
    case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
        return GLProgram::VarType::ISamplerCubemapArray;
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
        return GLProgram::VarType::ISampler2DMultiSample;
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::ISampler2DMultiSampleArray;
    case GL_INT_SAMPLER_BUFFER:
        return GLProgram::VarType::ISamplerBuffer;
    case GL_INT_SAMPLER_2D_RECT:
        return GLProgram::VarType::ISampler2DRect;
    case GL_UNSIGNED_INT_SAMPLER_1D:
        return GLProgram::VarType::USampler1D;
    case GL_UNSIGNED_INT_SAMPLER_2D:
        return GLProgram::VarType::USampler2D;
    case GL_UNSIGNED_INT_SAMPLER_3D:
        return GLProgram::VarType::USampler3D;
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return GLProgram::VarType::USamplerCubemap;
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        return GLProgram::VarType::USampler1DArray;
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        return GLProgram::VarType::USampler2DArray;
    case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
        return GLProgram::VarType::USamplerCubemapArray;
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        return GLProgram::VarType::USampler2DMultiSample;
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::USampler2DMultiSampleArray;
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        return GLProgram::VarType::USamplerBuffer;
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
        return GLProgram::VarType::USampler2DRect;
    case GL_IMAGE_1D:
        return GLProgram::VarType::Image1D;
    case GL_IMAGE_2D:
        return GLProgram::VarType::Image2D;
    case GL_IMAGE_3D:
        return GLProgram::VarType::Image3D;
    case GL_IMAGE_2D_RECT:
        return GLProgram::VarType::Image2DRect;
    case GL_IMAGE_CUBE:
        return GLProgram::VarType::ImageCubemap;
    case GL_IMAGE_BUFFER:
        return GLProgram::VarType::ImageBuffer;
    case GL_IMAGE_1D_ARRAY:
        return GLProgram::VarType::Image1DArray;
    case GL_IMAGE_2D_ARRAY:
        return GLProgram::VarType::Image2DArray;
    case GL_IMAGE_CUBE_MAP_ARRAY:
        return GLProgram::VarType::ImageCubemapArray;
    case GL_IMAGE_2D_MULTISAMPLE:
        return GLProgram::VarType::Image2DMultiSample;
    case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::Image2DMultiSampleArray;
    case GL_INT_IMAGE_1D:
        return GLProgram::VarType::IImage1D;
    case GL_INT_IMAGE_2D:
        return GLProgram::VarType::IImage2D;
    case GL_INT_IMAGE_3D:
        return GLProgram::VarType::IImage3D;
    case GL_INT_IMAGE_2D_RECT:
        return GLProgram::VarType::IImage2DRect;
    case GL_INT_IMAGE_CUBE:
        return GLProgram::VarType::IImageCubemap;
    case GL_INT_IMAGE_BUFFER:
        return GLProgram::VarType::IImageBuffer;
    case GL_INT_IMAGE_1D_ARRAY:
        return GLProgram::VarType::IImage1DArray;
    case GL_INT_IMAGE_2D_ARRAY:
        return GLProgram::VarType::IImage2DArray;
    case GL_INT_IMAGE_CUBE_MAP_ARRAY:
        return GLProgram::VarType::IImageCubemapArray;
    case GL_INT_IMAGE_2D_MULTISAMPLE:
        return GLProgram::VarType::IImage2DMultiSample;
    case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::IImage2DMultiSampleArray;
    case GL_UNSIGNED_INT_IMAGE_1D:
        return GLProgram::VarType::UImage1D;
    case GL_UNSIGNED_INT_IMAGE_2D:
        return GLProgram::VarType::UImage2D;
    case GL_UNSIGNED_INT_IMAGE_3D:
        return GLProgram::VarType::UImage3D;
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
        return GLProgram::VarType::UImage2DRect;
    case GL_UNSIGNED_INT_IMAGE_CUBE:
        return GLProgram::VarType::UImageCubemap;
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
        return GLProgram::VarType::UImageBuffer;
    case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
        return GLProgram::VarType::UImage1DArray;
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
        return GLProgram::VarType::UImage2DArray;
    case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
        return GLProgram::VarType::UImageCubemapArray;
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
        return GLProgram::VarType::UImage2DMultiSample;
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
        return GLProgram::VarType::UImage2DMultiSampleArray;
    case GL_UNSIGNED_INT_ATOMIC_COUNTER:
        return GLProgram::VarType::AtomicCounter;
    }

    std::cerr << std::format("Unknown shader variable type: {:#x}\n", static_cast<unsigned int>(type));

    return GLProgram::VarType::Unknown;
}

GLProgram::GLProgram(std::vector<ShaderModule> const& shaderModules) {
    // create program and attach/link shader modules
    m_handle = glCreateProgram();
    checkGLErrors();
    if (m_handle == 0) {
        throw std::runtime_error("Create OpenGL program failure");
    }

    for (auto const& shaderModule : shaderModules) {
        attach(shaderModule);
    }

    link();

    for (auto const& shaderModule : shaderModules) {
        detach(shaderModule);
    }

    queryResourceMetaInfos();
#ifndef NDEBUG
    // printResourceInfos();
#endif
}

GLProgram::GLProgram(
    std::vector<ShaderModule> const& shaderModules,
    std::map<ShaderModule::Stage, UniformVarInfoMap> const& uniformVarStageInfos,
    std::map<ShaderModule::Stage, TextureInfoMap> const& textureStageInfos,
    std::map<ShaderModule::Stage, UniformBufferInfoMap> const& uniformBufferStageInfos,
    std::map<ShaderModule::Stage, StorageBufferInfoMap> const& storageBufferStageInfos,
    std::map<ShaderModule::Stage, StorageImageInfoMap> const& storageImageStageInfos,
    std::map<ShaderModule::Stage, AtomicCounterInfoMap> const& atomicCounterStageInfos,
    TransformFeedbackBufferInfoMap const& transformFeedbackBufferInfos)
    : m_transformFeedbackBufferInfos{transformFeedbackBufferInfos} {
    // create program and attach/link shader modules
    m_handle = glCreateProgram();
    checkGLErrors();
    if (m_handle == 0) {
        throw std::runtime_error("Create OpenGL program failure");
    }

    for (auto const& shaderModule : shaderModules) {
        attach(shaderModule);
    }

    link();

    for (auto const& shaderModule : shaderModules) {
        detach(shaderModule);
    }

    // assemble reflection data from different stage
    // note: in OpenGL, we don't care about different stage, so combine them all together
    for (auto const& [stage, varInfos] : uniformVarStageInfos) {
        for (auto const& [name, varInfo] : varInfos) {
            if (m_uniformVarInfos.find(name) == m_uniformVarInfos.end()) {
                m_uniformVarInfos[name] = varInfo;
            }
        }
    }

    for (auto const& [stage, textureInfos] : textureStageInfos) {
        for (auto const& [name, textureInfo] : textureInfos) {
            if (m_textureInfos.find(name) == m_textureInfos.end()) {
                m_textureInfos[name] = textureInfo;
            }
        }
    }

    for (auto const& [stage, blockInfos] : uniformBufferStageInfos) {
        for (auto const& [name, blockInfo] : blockInfos) {
            if (m_uniformBufferInfos.find(name) == m_uniformBufferInfos.end()) {
                m_uniformBufferInfos[name] = blockInfo;
            }
        }
    }

    for (auto const& [stage, bufferInfos] : storageBufferStageInfos) {
        for (auto const& [name, bufferInfo] : bufferInfos) {
            if (m_storageBufferInfos.find(name) == m_storageBufferInfos.end()) {
                m_storageBufferInfos[name] = bufferInfo;
            }
        }
    }

    for (auto const& [stage, imageInfos] : storageImageStageInfos) {
        for (auto const& [name, imageInfo] : imageInfos) {
            if (m_storageImageInfos.find(name) == m_storageImageInfos.end()) {
                m_storageImageInfos[name] = imageInfo;
            }
        }
    }

    for (auto const& [stage, atomicInfos] : atomicCounterStageInfos) {
        for (auto const& [name, atomicInfo] : atomicInfos) {
            if (m_atomicCounterInfos.find(name) == m_atomicCounterInfos.end()) {
                m_atomicCounterInfos[name] = atomicInfo;
            }
        }
    }

    queryTransformFeedbackInfo();
}

GLProgram::~GLProgram() {
    if (isValid()) {
        glDeleteProgram(m_handle);
    }
}

GLProgram& GLProgram::operator=(GLProgram&& rhs) noexcept {
    if (this != &rhs) {
        if (isValid()) {
            glDeleteProgram(m_handle);
        }

        m_uniformVarInfos = std::move(rhs.m_uniformVarInfos);
        m_textureInfos = std::move(rhs.m_textureInfos);
        m_uniformBufferInfos = std::move(rhs.m_uniformBufferInfos);
        m_storageBufferInfos = std::move(rhs.m_storageBufferInfos);
        m_storageImageInfos = std::move(rhs.m_storageImageInfos);
        m_atomicCounterInfos = std::move(rhs.m_atomicCounterInfos);
        m_transformFeedbackBufferInfos = std::move(rhs.m_transformFeedbackBufferInfos);

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

void GLProgram::use() const {
    glUseProgram(m_handle);
}

void GLProgram::unuse() {
    glUseProgram(0);
}

int GLProgram::getUniformVarLocation(std::string_view name) const {
    auto it = m_uniformVarInfos.find(name);
    return it == m_uniformVarInfos.end() ? -1 : it->second.location;
}

int GLProgram::getTextureBinding(std::string_view name) const {
    for (auto const& [tName, info] : m_textureInfos) {
        if (tName == name) {
            return info.binding;
        }
    }

    return -1;
}

void GLProgram::setUniform(int location, bool const& value) const {
    glProgramUniform1i(m_handle, location, static_cast<int>(value));
}

void GLProgram::setUniform(int location, glm::bvec2 const& value) const {
    glProgramUniform2i(m_handle, location, static_cast<int>(value.x), static_cast<int>(value.y));
}

void GLProgram::setUniform(int location, glm::bvec3 const& value) const {
    glProgramUniform3i(
        m_handle, location, static_cast<int>(value.x), static_cast<int>(value.y), static_cast<int>(value.z));
}

void GLProgram::setUniform(int location, glm::bvec4 const& value) const {
    glProgramUniform4i(
        m_handle,
        location,
        static_cast<int>(value.x),
        static_cast<int>(value.y),
        static_cast<int>(value.z),
        static_cast<int>(value.w));
}

void GLProgram::setUniform(int location, int const& value) const {
    glProgramUniform1i(m_handle, location, value);
}

void GLProgram::setUniform(int location, glm::ivec2 const& value) const {
    glProgramUniform2iv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::ivec3 const& value) const {
    glProgramUniform3iv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::ivec4 const& value) const {
    glProgramUniform4iv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, uint32_t const& value) const {
    glProgramUniform1ui(m_handle, location, value);
}

void GLProgram::setUniform(int location, glm::uvec2 const& value) const {
    glProgramUniform2uiv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::uvec3 const& value) const {
    glProgramUniform3uiv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::uvec4 const& value) const {
    glProgramUniform4uiv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, float const& value) const {
    glProgramUniform1f(m_handle, location, value);
}

void GLProgram::setUniform(int location, glm::vec2 const& value) const {
    glProgramUniform2fv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::vec3 const& value) const {
    glProgramUniform3fv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::vec4 const& value) const {
    glProgramUniform4fv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, double const& value) const {
    glProgramUniform1d(m_handle, location, value);
}

void GLProgram::setUniform(int location, glm::dvec2 const& value) const {
    glProgramUniform2dv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::dvec3 const& value) const {
    glProgramUniform3dv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::dvec4 const& value) const {
    glProgramUniform4dv(m_handle, location, 1, &value[0]);
}

void GLProgram::setUniform(int location, glm::mat2x2 const& value) const {
    glProgramUniformMatrix2fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat2x3 const& value) const {
    glProgramUniformMatrix2x3fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat2x4 const& value) const {
    glProgramUniformMatrix2x4fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat3x2 const& value) const {
    glProgramUniformMatrix3x2fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat3x3 const& value) const {
    glProgramUniformMatrix3fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat3x4 const& value) const {
    glProgramUniformMatrix3x4fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat4x2 const& value) const {
    glProgramUniformMatrix4x2fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat4x3 const& value) const {
    glProgramUniformMatrix4x3fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::mat4x4 const& value) const {
    glProgramUniformMatrix4fv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat2x2 const& value) const {
    glProgramUniformMatrix2dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat2x3 const& value) const {
    glProgramUniformMatrix2x3dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat2x4 const& value) const {
    glProgramUniformMatrix2x4dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat3x2 const& value) const {
    glProgramUniformMatrix3x2dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat3x3 const& value) const {
    glProgramUniformMatrix3dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat3x4 const& value) const {
    glProgramUniformMatrix3x4dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat4x2 const& value) const {
    glProgramUniformMatrix4x2dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat4x3 const& value) const {
    glProgramUniformMatrix4x3dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::setUniform(int location, glm::dmat4x4 const& value) const {
    glProgramUniformMatrix4dv(m_handle, location, 1, GL_FALSE, &value[0][0]);
}

void GLProgram::printResourceInfos() const {
    std::cout << "Resource Infos\n";
    printUniformVarInfos();
    printTextureInfos();
    printUniformBlockInfos();
    printStorageBufferInfos();
    printStorageImageInfos();
    printAtomicCounterInfos();
    printTransformFeedbackInfo();
}

void GLProgram::attach(ShaderModule const& shaderModule) {
    glAttachShader(m_handle, shaderModule.getNativeHandle());
}

void GLProgram::detach(ShaderModule const& shaderModule) {
    glDetachShader(m_handle, shaderModule.getNativeHandle());
}

void GLProgram::link() {
    glLinkProgram(m_handle);

    GLint result = GL_FALSE;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        GLint infoLogLength{0};
        glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> buffer(infoLogLength);
        glGetProgramInfoLog(m_handle, infoLogLength, NULL, &buffer[0]);
        throw std::runtime_error("Link error: " + std::string(buffer.data()));
    }
}

void GLProgram::queryResourceMetaInfos() {
    // Every interesting reference
    // https://www.geometrictools.com/GTE/Graphics/GL45/GLSLReflection.cpp
    queryGenericUniforms();
    queryUniformBlocks();
    queryShaderStorageBuffers();
    queryAtomicCounters();
    queryTransformFeedbackInfo();
}

void GLProgram::queryTransformFeedbackInfo() {
    GLint bufferCount{};
    glGetProgramInterfaceiv(m_handle, GL_TRANSFORM_FEEDBACK_BUFFER, GL_ACTIVE_RESOURCES, &bufferCount);

    GLenum const bufferProperties[]{GL_BUFFER_BINDING, GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE};
    constexpr GLsizei bufferPropertyCount{static_cast<GLsizei>(std::size(bufferProperties))};
    for (GLint i = 0; i < bufferCount; ++i) {
        GLint results[bufferPropertyCount]{};
        glGetProgramResourceiv(
            m_handle,
            GL_TRANSFORM_FEEDBACK_BUFFER,
            static_cast<GLuint>(i),
            bufferPropertyCount,
            bufferProperties,
            bufferPropertyCount,
            nullptr,
            results);

        GLint const bufferBinding{results[0]};
        GLint const stride{results[1]};
        if (bufferBinding < 0 || stride < 0) {
            continue;
        }
        m_transformFeedbackBufferInfos.try_emplace(
            bufferBinding,
            TransformFeedbackBufferInfo{
                .stride = stride,
            });
    }

    GLint varyingCount{};
    glGetProgramInterfaceiv(m_handle, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, &varyingCount);

    GLenum const varyingProperties[]{
        GL_NAME_LENGTH,
        GL_TYPE,
        GL_ARRAY_SIZE,
        GL_OFFSET,
        GL_TRANSFORM_FEEDBACK_BUFFER_INDEX,
    };
    constexpr GLsizei varyingPropertyCount{static_cast<GLsizei>(std::size(varyingProperties))};
    for (GLint i = 0; i < varyingCount; ++i) {
        GLint results[varyingPropertyCount]{};
        glGetProgramResourceiv(
            m_handle,
            GL_TRANSFORM_FEEDBACK_VARYING,
            static_cast<GLuint>(i),
            varyingPropertyCount,
            varyingProperties,
            varyingPropertyCount,
            nullptr,
            results);

        GLint const nameLength{results[0]};
        GLint const arraySize{results[2]};
        GLint const offset{results[3]};
        GLint const bufferBinding{results[4]};
        if (nameLength <= 0 || arraySize < 0 || offset < 0 || bufferBinding < 0) {
            continue;
        }

        std::vector<GLchar> nameBuffer(static_cast<std::string::size_type>(nameLength));
        GLsizei writtenNameLength{};
        glGetProgramResourceName(
            m_handle,
            GL_TRANSFORM_FEEDBACK_VARYING,
            static_cast<GLuint>(i),
            nameLength,
            &writtenNameLength,
            nameBuffer.data());
        std::string name(nameBuffer.data(), static_cast<std::string::size_type>(writtenNameLength));

        VarType const type{results[1] == GL_NONE ? VarType::Unknown : getVarType(results[1])};
        auto const bufferIt{m_transformFeedbackBufferInfos.find(bufferBinding)};
        if (bufferIt == m_transformFeedbackBufferInfos.end()) {
            continue;
        }

        bufferIt->second.varyings.try_emplace(
            std::move(name),
            TransformFeedbackVaryingInfo{
                .type = type,
                .arraySize = static_cast<uint32_t>(arraySize),
                .offset = offset,
            });
    }
}

void GLProgram::queryGenericUniforms() {
    // Generic uniforms including
    // + plain uniform varibles
    // + textures
    // + images
    // + atomic counters ???
    GLint uniformCount{};
    glGetProgramInterfaceiv(m_handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformCount);

    GLenum properties[]{GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
    for (GLint i = 0; i < uniformCount; ++i) {
        GLint results[4];
        glGetProgramResourceiv(m_handle, GL_UNIFORM, i, 4, properties, 4, nullptr, results);

        // Skip uniforms in uniform blocks
        if (results[3] != -1) {
            continue;
        }

        GLint nameLen = results[0];
        std::string name(nameLen - 1, '\0');
        glGetProgramResourceName(m_handle, GL_UNIFORM, i, nameLen, nullptr, name.data());

        int location = results[2];
        VarType varType = getVarType(results[1]);
        if (varType == GLProgram::VarType::Unknown) {
            std::cerr << "Cannot get shader variable type of " << name << '\n';
        }

        if (isTexture(varType)) {
            GLint binding;
            glGetUniformiv(m_handle, location, &binding);
            m_textureInfos.try_emplace(std::move(name), binding, varType);
        }
        else if (isImage(varType)) {
            GLint binding;
            glGetUniformiv(m_handle, location, &binding);
            m_storageImageInfos.try_emplace(std::move(name), binding, varType);
        }
        else {
            m_uniformVarInfos.try_emplace(std::move(name), location, varType);
        }
    }

    // We reassign all texture binding if all textures bindings are 0
    // Since we use binding instead of location for textures
    if (!m_textureInfos.empty()) {
        bool allZero = true;
        for (auto const& [name, info] : m_textureInfos) {
            if (info.binding != 0) {
                allZero = false;
                break;
            }
        }

        if (allZero) {
            int nextBinding = 0;
            for (auto& [name, info] : m_textureInfos) {
                GLint const location = glGetUniformLocation(m_handle, name.c_str());
                if (location < 0) {
                    continue;
                }

                glProgramUniform1i(m_handle, location, nextBinding);
                info.binding = nextBinding;
                ++nextBinding;
            }
        }
    }
}

void GLProgram::queryUniformBlocks() {
    GLint blockCount = 0;
    glGetProgramInterfaceiv(m_handle, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &blockCount);

    GLenum properties[] = {GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE};
    for (GLint i = 0; i < blockCount; ++i) {
        GLint results[3];
        glGetProgramResourceiv(m_handle, GL_UNIFORM_BLOCK, i, 3, properties, 3, nullptr, results);

        GLint nameLen = results[0];
        std::string name(nameLen - 1, '\0');
        glGetProgramResourceName(m_handle, GL_UNIFORM_BLOCK, i, nameLen, nullptr, name.data());

        UniformBufferInfo uboInfo{
            .binding = results[1],
            .size = static_cast<uint32_t>(results[2]),
        };

        m_uniformBufferInfos.emplace(std::move(name), uboInfo);
    }
}

void GLProgram::queryShaderStorageBuffers() {
    GLint ssboCount = 0;
    glGetProgramInterfaceiv(m_handle, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &ssboCount);

    GLenum properties[] = {GL_NAME_LENGTH, GL_BUFFER_BINDING};
    for (GLint i = 0; i < ssboCount; ++i) {
        GLint results[2];
        glGetProgramResourceiv(m_handle, GL_SHADER_STORAGE_BLOCK, i, 2, properties, 2, nullptr, results);

        GLint nameLen = results[0];
        std::string name(nameLen - 1, '\0');
        glGetProgramResourceName(m_handle, GL_SHADER_STORAGE_BLOCK, i, nameLen, nullptr, name.data());

        StorageBufferInfo ssboInfo{.binding = results[1]};

        m_storageBufferInfos.emplace(std::move(name), ssboInfo);
    }
}

void GLProgram::queryAtomicCounters() {
    // As we need get more specific infos of atomic counter buffer, we don't use the following API,
    // as it cannot provide name and offset infos of atomic buffer
    // GLint bufferCount = 0;
    // glGetProgramInterfaceiv(m_handle, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &bufferCount);

    // GLenum properties[] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE };
    // for (int i = 0; i < bufferCount; ++i) {
    //     GLint results[2];
    //     glGetProgramResourceiv(m_handle, GL_ATOMIC_COUNTER_BUFFER, i, 2, properties, 2, NULL, results);

    //    GLint bufferBinding = results[0];
    //    GLint dataSize = results[1];
    //}

    GLint numUniforms{};
    glGetProgramiv(m_handle, GL_ACTIVE_UNIFORMS, &numUniforms);

    GLint maxLength{};
    glGetProgramiv(m_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    std::unique_ptr<GLchar[]> name{new GLchar[maxLength]};

    for (GLint i = 0; i < numUniforms; ++i) {
        GLint size;
        GLenum type;
        glGetActiveUniform(m_handle, i, maxLength, NULL, &size, &type, name.get());
        if (type == GL_UNSIGNED_INT_ATOMIC_COUNTER) {
            GLint index;
            glGetActiveUniformsiv(m_handle, 1, (GLuint const*)&i, GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX, &index);
            if (index < 0) {
                std::cerr << "Get atomic counter buffer variable " << name.get() << " info failure\n";
                continue;
            }

            GLint binding;
            glGetActiveAtomicCounterBufferiv(m_handle, index, GL_ATOMIC_COUNTER_BUFFER_BINDING, &binding);

            GLint offset;
            glGetActiveUniformsiv(m_handle, 1, (GLuint const*)&i, GL_UNIFORM_OFFSET, &offset);

            m_atomicCounterInfos.try_emplace(std::string(name.get()), binding, offset);
        }
    }
}

void GLProgram::printUniformVarInfos() const {
    std::cout << "+ Uniform Var Infos\n";
    for (auto const& [name, varInfo] : m_uniformVarInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + location: {}\n", varInfo.location);
        std::cout << std::format("    + type:     {}\n", getVarTypeName(varInfo.type));
    }
}

void GLProgram::printTextureInfos() const {
    std::cout << "+ Texture Infos\n";
    for (auto const& [name, texInfo] : m_textureInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + binding: {}\n", texInfo.binding);
        std::cout << std::format("    + type:    {}\n", getVarTypeName(texInfo.type));
    }
}

void GLProgram::printUniformBlockInfos() const {
    std::cout << "+ Uniform Block Infos\n";
    for (auto const& [name, uboInfo] : m_uniformBufferInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + binding: {}\n", uboInfo.binding);
        std::cout << std::format("    + size:    {}\n", uboInfo.size);
    }
}

void GLProgram::printStorageBufferInfos() const {
    std::cout << "+ Storage Buffer Infos\n";
    for (auto const& [name, ssboInfo] : m_storageBufferInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + binding: {}\n", ssboInfo.binding);
    }
}

void GLProgram::printStorageImageInfos() const {
    std::cout << "+ Storage Image Infos\n";
    for (auto const& [name, imageInfo] : m_storageImageInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + binding: {}\n", imageInfo.binding);
        std::cout << std::format("    + type:    {}\n", getVarTypeName(imageInfo.type));
    }
}

void GLProgram::printAtomicCounterInfos() const {
    std::cout << "+ Atomic Counter Infos\n";
    for (auto const& [name, atomicInfo] : m_atomicCounterInfos) {
        std::cout << std::format("  + {}\n", name);
        std::cout << std::format("    + binding: {}\n", atomicInfo.binding);
        std::cout << std::format("    + offset:  {}\n", atomicInfo.offset);
    }
}

void GLProgram::printTransformFeedbackInfo() const {
    std::cout << "+ Transform Feedback Infos\n";
    for (auto const& [binding, bufferInfo] : m_transformFeedbackBufferInfos) {
        std::cout << std::format("  + buffer binding {}\n", binding);
        std::cout << std::format("    + stride: {}\n", bufferInfo.stride);
        for (auto const& [name, varyingInfo] : bufferInfo.varyings) {
            std::cout << std::format("    + {}\n", name);
            std::cout << std::format("      + type:       {}\n", getVarTypeName(varyingInfo.type));
            std::cout << std::format("      + array size: {}\n", varyingInfo.arraySize);
            std::cout << std::format("      + offset:     {}\n", varyingInfo.offset);
        }
    }
}

bool GLProgram::isTexture(VarType type) noexcept {
    switch (type) {
    default:
        break;
    case VarType::Sampler1D:
    case VarType::Sampler2D:
    case VarType::Sampler3D:
    case VarType::SamplerCubemap:
    case VarType::Sampler1DShadow:
    case VarType::Sampler2DShadow:
    case VarType::Sampler1DArray:
    case VarType::Sampler2DArray:
    case VarType::SamplerCubeArray:
    case VarType::Sampler1DArrayShadow:
    case VarType::Sampler2DArrayShadow:
    case VarType::Sampler2DMultiSample:
    case VarType::Sampler2DMultiSampleArray:
    case VarType::SamplerCubemapShadow:
    case VarType::SamplerCubemapArrayShadow:
    case VarType::SamplerBuffer:
    case VarType::Sampler2DRect:
    case VarType::Sampler2DRectShadow:
    case VarType::ISampler1D:
    case VarType::ISampler2D:
    case VarType::ISampler3D:
    case VarType::ISamplerCubemap:
    case VarType::ISampler1DArray:
    case VarType::ISampler2DArray:
    case VarType::ISamplerCubemapArray:
    case VarType::ISampler2DMultiSample:
    case VarType::ISampler2DMultiSampleArray:
    case VarType::ISamplerBuffer:
    case VarType::ISampler2DRect:
    case VarType::USampler1D:
    case VarType::USampler2D:
    case VarType::USampler3D:
    case VarType::USamplerCubemap:
    case VarType::USampler1DArray:
    case VarType::USampler2DArray:
    case VarType::USamplerCubemapArray:
    case VarType::USampler2DMultiSample:
    case VarType::USampler2DMultiSampleArray:
    case VarType::USamplerBuffer:
    case VarType::USampler2DRect:
        return true;
    }

    return false;
}

bool GLProgram::isImage(VarType type) noexcept {
    switch (type) {
    default:
        break;
    case VarType::Image1D:
    case VarType::Image2D:
    case VarType::Image3D:
    case VarType::Image2DRect:
    case VarType::ImageCubemap:
    case VarType::ImageBuffer:
    case VarType::Image1DArray:
    case VarType::Image2DArray:
    case VarType::ImageCubemapArray:
    case VarType::Image2DMultiSample:
    case VarType::Image2DMultiSampleArray:
    case VarType::IImage1D:
    case VarType::IImage2D:
    case VarType::IImage3D:
    case VarType::IImage2DRect:
    case VarType::IImageCubemap:
    case VarType::IImageBuffer:
    case VarType::IImage1DArray:
    case VarType::IImage2DArray:
    case VarType::IImageCubemapArray:
    case VarType::IImage2DMultiSample:
    case VarType::IImage2DMultiSampleArray:
    case VarType::UImage1D:
    case VarType::UImage2D:
    case VarType::UImage3D:
    case VarType::UImage2DRect:
    case VarType::UImageCubemap:
    case VarType::UImageBuffer:
    case VarType::UImage1DArray:
    case VarType::UImage2DArray:
    case VarType::UImageCubemapArray:
    case VarType::UImage2DMultiSample:
    case VarType::UImage2DMultiSampleArray:
        return true;
    }

    return false;
}
} // namespace gfx

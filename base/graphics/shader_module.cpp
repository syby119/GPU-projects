#include "graphics/shader_module.h"

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "graphics/gl_utility.h"

namespace gfx {

static std::string_view getStageName(ShaderModule::Stage stage) noexcept {
    switch (stage) {
    case ShaderModule::Stage::Vertex:
        return "vertex";
    case ShaderModule::Stage::TessControl:
        return "tessellation control";
    case ShaderModule::Stage::TessEvaluation:
        return "tessellation evaluation";
    case ShaderModule::Stage::Geometry:
        return "geometry";
    case ShaderModule::Stage::Fragment:
        return "fragment";
    case ShaderModule::Stage::Compute:
        return "compute";
    case ShaderModule::Stage::Task:
        return "task";
    case ShaderModule::Stage::Mesh:
        return "mesh";
    case ShaderModule::Stage::RayGen:
        return "ray generation";
    case ShaderModule::Stage::RayIntersection:
        return "ray intersection";
    case ShaderModule::Stage::RayAnyHit:
        return "ray any-hit";
    case ShaderModule::Stage::RayClosestHit:
        return "ray closest-hit";
    case ShaderModule::Stage::RayMiss:
        return "ray miss";
    case ShaderModule::Stage::RayCallable:
        return "ray callable";
    }

    return "unknown";
}

static GLenum toNativeStage(ShaderModule::Stage stage) {
    switch (stage) {
    case ShaderModule::Stage::Vertex:
        return GL_VERTEX_SHADER;
    case ShaderModule::Stage::TessControl:
        return GL_TESS_CONTROL_SHADER;
    case ShaderModule::Stage::TessEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    case ShaderModule::Stage::Geometry:
        return GL_GEOMETRY_SHADER;
    case ShaderModule::Stage::Fragment:
        return GL_FRAGMENT_SHADER;
    case ShaderModule::Stage::Compute:
        return GL_COMPUTE_SHADER;
    case ShaderModule::Stage::Task:
        return GL_TASK_SHADER_NV;
    case ShaderModule::Stage::Mesh:
        return GL_MESH_SHADER_NV;
    default:
        // OpenGL does not support raytracing pipeline
        break;
    }

    throw std::runtime_error("Unsupported shader stage: " + std::string(getStageName(stage)));
}

ShaderModule::ShaderModule(std::string const& code, Stage stage)
    : m_stage{stage} {
    m_handle = glCreateShader(toNativeStage(stage));
    checkGLErrors();

    if (m_handle == 0) {
        throw std::runtime_error("Create OpenGL " + std::string(getStageName(stage)) + " shader failure");
    }

    char const* codeBuf = code.c_str();
    glShaderSource(m_handle, 1, &codeBuf, nullptr);
    glCompileShader(m_handle);

    GLint success;
    glGetShaderiv(m_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length{};
        glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &length);

        std::string message{"Compile error: \n"};
        if (length > 0) {
            std::vector<GLchar> buffer(length);
            glGetShaderInfoLog(m_handle, length, nullptr, buffer.data());
            message += buffer.data();
        }

        std::cerr << '\n' << code << '\n';
        throw std::runtime_error(message);
    }
}

ShaderModule::ShaderModule(std::vector<uint32_t> const& spirv, Stage stage, char const* entrypoint)
    : m_stage{stage} {
    m_handle = glCreateShader(toNativeStage(stage));
    checkGLErrors();

    if (m_handle == 0) {
        throw std::runtime_error("Create OpenGL " + std::string(getStageName(stage)) + " shader failure");
    }

    glShaderBinary(
        1,
        &m_handle,
        GL_SHADER_BINARY_FORMAT_SPIR_V,
        spirv.data(),
        static_cast<GLsizei>(spirv.size() * sizeof(uint32_t)));
    glSpecializeShader(m_handle, entrypoint, 0, nullptr, nullptr);
}

ShaderModule::~ShaderModule() {
    if (m_handle) {
        glDeleteShader(m_handle);
    }
}

ShaderModule& ShaderModule::operator=(ShaderModule&& rhs) noexcept {
    if (this != &rhs) {
        if (isValid()) {
            glDeleteShader(m_handle);
        }

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

} // namespace gfx

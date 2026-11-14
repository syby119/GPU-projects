#include "graphics/graphics_context.h"

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "graphics/gl_utility.h"

// Prefer discrete GPU
#if defined(_WIN32)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace gfx {

GraphicsContext::GraphicsContext(std::pair<int, int> const& minimumVersion)
    : m_driverVersion{4, 5} {
    constexpr std::array validVersions{
        std::pair{1, 0}, std::pair{1, 1}, std::pair{1, 2}, std::pair{1, 3}, std::pair{1, 4},
        std::pair{1, 5}, std::pair{2, 0}, std::pair{2, 1}, std::pair{3, 0}, std::pair{3, 1},
        std::pair{3, 2}, std::pair{3, 3}, std::pair{4, 0}, std::pair{4, 1}, std::pair{4, 2},
        std::pair{4, 3}, std::pair{4, 4}, std::pair{4, 5}, std::pair{4, 6},
    };

    if (std::find(validVersions.begin(), validVersions.end(), minimumVersion) == validVersions.end()) {
        std::cerr << std::format(
            "OpenGL {}.{} is not a valid desktop OpenGL version;\n"
            "Using OpenGL {}.{} instead.\n",
            minimumVersion.first,
            minimumVersion.second,
            m_driverVersion.first,
            m_driverVersion.second);
    }
    else if (minimumVersion < m_driverVersion) {
        std::cerr << std::format(
            "OpenGL {}.{} is below the required version for DSA support;\n"
            "Using OpenGL {}.{} instead.\n",
            minimumVersion.first,
            minimumVersion.second,
            m_driverVersion.first,
            m_driverVersion.second);
    }
    else {
        m_driverVersion = minimumVersion;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, m_driverVersion.first);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, m_driverVersion.second);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
}

void GraphicsContext::init(GLFWwindow* window) {
    glfwMakeContextCurrent(window);

    // load OpenGL library functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("glad initialization OpenGL failure");
    }

    if (!checkVersion(window)) {
        throw std::runtime_error("Minimum OpenGL version does not satisfied");
    }

    std::cout << "OpenGL\n";
    std::cout << std::format("+ version:     {}\n", reinterpret_cast<char const*>(glGetString(GL_VERSION)));
    std::cout << std::format("+ renderer:    {}\n", reinterpret_cast<char const*>(glGetString(GL_RENDERER)));
    std::cout << std::format(
        "+ glsl:        {}\n", reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    GLint maxBlockSize{};
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBlockSize);
    std::cout << std::format("+ ubo limit:   {} bytes\n", maxBlockSize);

    std::cout << std::format("+ max texture\n");
    GLint maxVertexTextureUint{};
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUint);
    std::cout << std::format("  + vertex:    {}\n", maxVertexTextureUint);

    GLint maxFragmentTextureUint{};
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragmentTextureUint);
    std::cout << std::format("  + fragment:  {}\n", maxFragmentTextureUint);

    GLint maxCombinedTextureUint{};
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUint);
    std::cout << std::format("  + total:     {}\n", maxCombinedTextureUint);

    std::cout << std::format("+ max image\n");
    GLint maxVertexImageUint{};
    glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &maxVertexImageUint);
    std::cout << std::format("  + vertex: {}\n", maxVertexImageUint);

    GLint maxFragmentImageUint{};
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &maxFragmentImageUint);
    std::cout << std::format("  + fragment: {}\n", maxFragmentImageUint);

    GLint maxTextureBufferSize{};
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferSize);
    std::cout << std::format("+ texture buffer limit: {} bytes\n\n", maxTextureBufferSize);

    // disable dither for performance
    glDisable(GL_DITHER);

    // always enable seamless cubemap sampling, to overcome cubemap edge sample artifacts
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    restoreDepthStates();

    restoreStencilStates();

#ifndef NDEBUG
    registerDebugOutput();
#endif
}

void GraphicsContext::restoreDepthStates() {
    // depth test is disabled by default
    glDisable(GL_DEPTH_TEST);
    // depth buffer clear value is 1.0f
    glClearDepth(1.0f);
    // depth value is writable
    glDepthMask(GL_TRUE);
    // depth compare function is less
    glDepthFunc(GL_LESS);
    // depth range is from [0.0f, 1.0f]
    glDepthRange(0.0f, 1.0f);
}

void GraphicsContext::restoreStencilStates() {
    // stencil test is disabled by default
    glDisable(GL_STENCIL_TEST);
    // stencil buffer clear value is 0
    glClearStencil(0);
    // all stencil bits are writable
    glStencilMask(0xFF);
    // stencil test is always, ref value is 0 and mask is OxFF
    // CompareFunc(StencilOp(ref, sValue) & mask)
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    // if CompareFunc tested is true, then keep the current value
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

bool GraphicsContext::checkVersion(GLFWwindow* window) {
    auto const majorVersion{glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR)};
    auto const minorVersion{glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR)};

    if (majorVersion == 0) {
        return false;
    }

    if (majorVersion < m_driverVersion.first ||
        (majorVersion == m_driverVersion.first && minorVersion < m_driverVersion.second)) {
        return false;
    }

    m_driverVersion.first = majorVersion;
    m_driverVersion.second = minorVersion;

    return true;
}

static void APIENTRY debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    GLchar const* message,
    void const* userParam) {
    // ignore these non-significant error codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    std::string sourceStr;
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "window system";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "shader compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "third party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "other";
        break;
    }

    std::string typeStr;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "deprecated behaviour";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "undefined behaviour";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        typeStr = "marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typeStr = "push group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typeStr = "pop group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "other";
        break;
    }

    std::string const msg{message};
    std::ostream* output = nullptr;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM:
        output = &std::cerr;
        break;
    case GL_DEBUG_SEVERITY_LOW:
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        output = &std::cout;
        break;
    }
    if (output != nullptr) {
        *output << std::format("OpenGL {} ({}): {}\n", sourceStr, typeStr, msg);
    }

#ifdef _MSC_VER
    if (type != GL_DEBUG_TYPE_MARKER && type != GL_DEBUG_TYPE_PERFORMANCE) {
        //__debugbreak();
    }
#endif
}

bool GraphicsContext::registerDebugOutput() {
    if (m_driverVersion.first < 4 || m_driverVersion.second < 3) {
        return false;
    }

    GLint flags{};
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);

    constexpr GLenum source{GL_DONT_CARE};
    constexpr GLenum type{GL_DONT_CARE};
    constexpr GLenum severity{GL_DONT_CARE};
    glDebugMessageControl(source, type, severity, 0, nullptr, GL_TRUE);

    checkGLErrors();

    return true;
}
} // namespace gfx

#include <optix_viewer.h>

#include <common/event/event_dispatcher.h>
#include <common/event/keyboard_event.h>
#include <common/event/window_event.h>
#include <gpu/utility.h>
#include <graphics/rhi.h>
#include <optix/utility.h>

#include <cuda_gl_interop.h>

#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void selectCudaDeviceForOpenGL() {
    int deviceCount = 0;
    CHECK_CUDA(cudaGetDeviceCount(&deviceCount));

    std::vector<int> devices(deviceCount);
    uint32_t glDeviceCount = 0;
    CHECK_CUDA(cudaGLGetDevices(&glDeviceCount, devices.data(), deviceCount, cudaGLDeviceListCurrentFrame));
    if (glDeviceCount == 0) {
        throw std::runtime_error("The OpenGL context is not on a CUDA-capable GPU");
    }

    CHECK_CUDA(cudaSetDevice(devices[0]));
    gpu::printCudaDeviceName(devices[0]);
}

static std::unique_ptr<gfx::GLProgram> createRenderScreenProgram() {
    constexpr char vertexShader[] = R"GLSL(
#version 450 core

const vec2 corners[3] = vec2[3](
    vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

void main() {
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)GLSL";

    constexpr char fragmentShader[] = R"GLSL(
#version 450 core

layout(location = 0) uniform uint uWidth;
layout(std430, binding = 0) readonly buffer Pixels { vec4 pixels[]; };
out vec4 color;

vec3 linearToSrgb(vec3 value) {
    vec3 v = clamp(value, 0.0, 1.0);
    return mix(1.055 * pow(v, vec3(1.0 / 2.4)) - 0.055,
               12.92 * v,
               lessThanEqual(v, vec3(0.0031308)));
}

void main() {
    uint index = uint(gl_FragCoord.y) * uWidth + uint(gl_FragCoord.x);
    color = vec4(linearToSrgb(pixels[index].rgb), pixels[index].a);
}
)GLSL";

    std::vector<gfx::ShaderModule> shaders;
    shaders.emplace_back(vertexShader, gfx::ShaderModule::Stage::Vertex);
    shaders.emplace_back(fragmentShader, gfx::ShaderModule::Stage::Fragment);

    return std::make_unique<gfx::GLProgram>(std::move(shaders));
}

OptixViewer::OptixViewer(Application::Config const& config)
    : Application(config) {
    m_cameraController.lookAt(m_camera);
    updateWindowTitle();
    selectCudaDeviceForOpenGL();
    CHECK_OPTIX(optixInit());
    m_context = std::make_unique<optix::DeviceContext>();
    uint32_t const rtCoreVersion = m_context->getRtCoreVersion();
    if (rtCoreVersion == 0) {
        std::cout << "RT Core version: Not supported\n";
    }
    else {
        std::cout << std::format("RT Core version: {}.{}\n", rtCoreVersion / 10, rtCoreVersion % 10);
    }

    m_scene = std::make_unique<Scene>(*m_context, config.assetRootDir);
    m_renderer = std::make_unique<OptixRenderer>(
        *m_context,
        *m_scene,
        config.assetRootDir / "optix/project2.optixir",
        m_window.framebufferWidth(),
        m_window.framebufferHeight());

    resizeOutput(m_window.framebufferWidth(), m_window.framebufferHeight());

    m_renderScreenProgram = createRenderScreenProgram();
    m_screenVertexArray = std::make_unique<gfx::VertexArray>();

    gfx::RHI::enableDepthTest(false);
    gfx::RHI::enableBlend(false);

    resetClock();
}

OptixViewer::~OptixViewer() {
    if (m_outputResource) {
        LOG_CUDA(cudaDeviceSynchronize());
        gfx::RHI::finish();
    }
}

void OptixViewer::handleEvent(Event& event) {
    EventDispatcher dispatcher{event};
    dispatcher.dispatch<WindowFramebufferResizeEvent>([this](WindowFramebufferResizeEvent& resized) {
        m_renderer->resize(resized.getWidth(), resized.getHeight());
        resizeOutput(resized.getWidth(), resized.getHeight());
        return false;
    });

    dispatcher.dispatch<KeyPressEvent>([this](KeyPressEvent& key) {
        if (key.getKeyCode() == KeyCode::Escape) {
            close();
        }
        else if (key.getKeyCode() == KeyCode::Space && !key.isRepeated()) {
            m_denoiserEnabled = !m_denoiserEnabled;
            updateWindowTitle();
        }
        return false;
    });
}

void OptixViewer::updateWindowTitle() {
    m_window.setTitle(m_denoiserEnabled ? "OptiX Ray Tracer - Denoiser ON" : "OptiX Ray Tracer - Denoiser OFF");
}

void OptixViewer::resizeOutput(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    size_t const bytes = static_cast<size_t>(width) * height * sizeof(float4);
    if (m_outputBuffer && m_outputWidth == width && m_outputHeight == height) {
        return;
    }

    if (m_outputResource) {
        CHECK_CUDA(cudaDeviceSynchronize());
        gfx::RHI::finish();
        m_outputResource.reset();
        m_outputBuffer.reset();
    }

    m_outputBuffer = std::make_unique<gfx::Buffer>(
        gfx::Buffer::UsageFlags::ShaderStorageBuffer, bytes, gfx::Buffer::StorageFlags::Dynamic);
    m_outputResource = std::make_unique<gpu::GraphicsBufferResource>(
        *m_outputBuffer, gpu::GraphicsResource::RegisterFlags::WriteDiscard);
    m_outputWidth = width;
    m_outputHeight = height;
    m_samples = 0;
}

void OptixViewer::updateCamera() {
    glm::vec3 const horizontalForward = m_cameraController.horizontalForward(m_camera);
    glm::vec3 const right = m_cameraController.horizontalRight(m_camera);
    glm::vec3 movement{};
    if (m_input.isKeyDown(KeyCode::W)) {
        movement += horizontalForward;
    }
    if (m_input.isKeyDown(KeyCode::S)) {
        movement -= horizontalForward;
    }
    if (m_input.isKeyDown(KeyCode::D)) {
        movement += right;
    }
    if (m_input.isKeyDown(KeyCode::A)) {
        movement -= right;
    }
    if (m_input.isKeyDown(KeyCode::E)) {
        movement += glm::vec3{0, 1, 0};
    }
    if (m_input.isKeyDown(KeyCode::Q)) {
        movement -= glm::vec3{0, 1, 0};
    }

    float const scroll = m_input.getMouseScrollY();
    if (scroll != 0) {
        movement += scroll * 2 * horizontalForward;
    }

    if (m_cameraController.move(m_camera, movement, 5.0f * (scroll != 0 ? 0.1f : getDeltaTime()))) {
        m_samples = 0;
    }
}

void OptixViewer::renderFrame() {
    uint32_t const width = m_window.framebufferWidth();
    uint32_t const height = m_window.framebufferHeight();
    // A minimized window can have an empty framebuffer.
    if (width == 0 || height == 0) {
        return;
    }

    // Camera movement resets progressive accumulation before this frame is rendered.
    updateCamera();

    // CUDA renders directly into the OpenGL output buffer while it is mapped.
    m_outputResource->map();
    size_t count = 0;
    float4* pixels = m_outputResource->getMappedPointer<float4>(&count);
    if (pixels == nullptr || count < static_cast<size_t>(width) * height) {
        throw std::runtime_error("CUDA/OpenGL output buffer is too small");
    }

    uint32_t const samplesPerFrame = m_denoiserEnabled && m_samples == 0 ? 4u : 1u;
    m_renderer->render(pixels, m_samples, m_camera, m_denoiserEnabled, samplesPerFrame);
    m_samples += samplesPerFrame;
    m_outputResource->unmap();

    // OpenGL reads the buffer after unmapping and presents it with a full-screen triangle.
    gfx::RHI::setViewport(gfx::Viewport(width, height));
    gfx::RHI::clear(true, false);
    m_renderScreenProgram->use();
    m_renderScreenProgram->setUniform(0, width);
    m_outputBuffer->bindAsShaderStorage(0);
    m_screenVertexArray->bind();
    gfx::RHI::draw(gfx::RHI::Primitive::Triangles, 0, 3);
    m_screenVertexArray->unbind();
    m_outputBuffer->unbindAsShaderStorage(0);
    m_renderScreenProgram->unuse();
}

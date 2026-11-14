#include <mandelbrot_viewer.h>

#include <format>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <graphics/rhi.h>
#include <gpu/utility.h>
#include <common/event/event_dispatcher.h>
#include <common/event/keyboard_event.h>
#include <common/event/window_event.h>

#include <cuda_gl_interop.h>

static void selectCudaDeviceForCurrentOpenGLContext() {
    int cudaDeviceCount = 0;
    CHECK_CUDA(cudaGetDeviceCount(&cudaDeviceCount));

    std::vector<int> devices(cudaDeviceCount);
    uint32_t glDeviceCount = 0;
    CHECK_CUDA(cudaGLGetDevices(
        &glDeviceCount, devices.data(), static_cast<uint32_t>(cudaDeviceCount), cudaGLDeviceListCurrentFrame));

    if (glDeviceCount == 0) {
        throw std::runtime_error(
            "No CUDA device is associated with the current OpenGL context.\n"
            "Run the program natively on a CUDA-capable GPU that also owns the OpenGL context.\n");
    }

    CHECK_CUDA(cudaSetDevice(devices[0]));
    gpu::printCudaDeviceName(devices[0]);
}

static std::unique_ptr<gfx::GLProgram> createRenderScreenProgram() {
    constexpr char vertexShaderCode[] = R"GLSL(
#version 450 core

const vec2 triangle[3] = vec2[3](
    vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

void main() {
    gl_Position = vec4(triangle[gl_VertexID], 0.0, 1.0);
}
)GLSL";

    constexpr char fragmentShaderCode[] = R"GLSL(
#version 450 core

layout(location = 0) uniform uint uWidth;
layout(std430, binding = 0) readonly buffer Pixels { vec4 pixels[]; };

out vec4 fragColor;

void main() {
    uint index = uint(gl_FragCoord.y) * uWidth + uint(gl_FragCoord.x);
    fragColor = pixels[index];
}
)GLSL";

    std::vector<gfx::ShaderModule> shaderModules;
    shaderModules.emplace_back(vertexShaderCode, gfx::ShaderModule::Stage::Vertex);
    shaderModules.emplace_back(fragmentShaderCode, gfx::ShaderModule::Stage::Fragment);
    return std::make_unique<gfx::GLProgram>(std::move(shaderModules));
}

/*************************************************************************************************/
/*                                         MandelbrotViewer                                      */
/*************************************************************************************************/
MandelbrotViewer::MandelbrotViewer(Config const& config)
    : Application(config.applicationConfig)
    , m_playback(MandelbrotCompute::durationSeconds(), config.antialias) {
    selectCudaDeviceForCurrentOpenGLContext();

    m_palette = std::make_unique<Palette>();
    m_mandelbrotCompute = std::make_unique<MandelbrotCompute>(
        static_cast<int>(m_window.framebufferWidth()),
        static_cast<int>(m_window.framebufferHeight()),
        m_startCenter,
        m_startZoom,
        config.center,
        config.zoom);
    recreateColorBuffer(m_window.framebufferWidth(), m_window.framebufferHeight());

    m_renderScreenProgram = createRenderScreenProgram();
    m_screenVertexArray = std::make_unique<gfx::VertexArray>();

    gfx::RHI::enableDepthTest(false);
    gfx::RHI::enableBlend(false);
    gfx::RHI::enableScissorTest(false);
    gfx::RHI::setClearColor({0.0f, 0.0f, 0.0f, 1.0f});

    // reset the clock to avoid a large delta time on the first frame
    resetClock();
}

MandelbrotViewer::~MandelbrotViewer() {
    // Complete both sides of the CUDA/OpenGL interop contract before the
    // graphics resources are destroyed.
    if (m_colorBufferResource) {
        LOG_CUDA(cudaDeviceSynchronize());
        gfx::RHI::finish();
    }
}

void MandelbrotViewer::handleEvent(Event& event) {
    EventDispatcher dispatcher{event};
    dispatcher.dispatch<WindowFramebufferResizeEvent>([this](WindowFramebufferResizeEvent& resizeEvent) {
        uint32_t const width = resizeEvent.getWidth();
        uint32_t const height = resizeEvent.getHeight();
        if (width > 0 && height > 0) {
            m_mandelbrotCompute->setReferenceViewport(static_cast<int>(width), static_cast<int>(height));
            recreateColorBuffer(width, height);
        }
        return false;
    });
    dispatcher.dispatch<KeyPressEvent>([this](KeyPressEvent& keyEvent) {
        if (!keyEvent.isRepeated() && m_playback.handleKey(keyEvent)) {
            close();
        }
        return false;
    });
}

void MandelbrotViewer::renderFrame() {
    uint32_t const width = m_window.framebufferWidth();
    uint32_t const height = m_window.framebufferHeight();
    if (width == 0 || height == 0) {
        return;
    }

    m_playback.advance(getDeltaTime());

    if (!m_colorBuffer || !m_colorBufferResource || !m_mandelbrotCompute || !m_palette) {
        throw std::logic_error("Mandelbrot renderer has not been initialized");
    }

    MandelbrotCompute::FrameInfo frameInfo;
    m_colorBufferResource->map();
    size_t mappedPixelCount = 0;
    float4* output = m_colorBufferResource->getMappedPointer<float4>(&mappedPixelCount);
    size_t const requiredPixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (output == nullptr || mappedPixelCount < requiredPixelCount) {
        throw std::runtime_error("Mapped SSBO is smaller than the CUDA Mandelbrot output");
    }

    m_palette->shift = m_playback.paletteShift();
    frameInfo = m_mandelbrotCompute->render(
        output,
        static_cast<int>(width),
        static_cast<int>(height),
        m_playback.timeSeconds(),
        m_playback.antialias(),
        *m_palette);
    m_colorBufferResource->unmap();

    gfx::RHI::setViewport(gfx::Viewport(width, height));
    gfx::RHI::clear(true, false);
    m_renderScreenProgram->use();
    m_renderScreenProgram->setUniform(0, width);
    m_colorBuffer->bindAsShaderStorage(0);
    m_screenVertexArray->bind();
    gfx::RHI::draw(gfx::RHI::Primitive::Triangles, 0, 3);
    m_screenVertexArray->unbind();
    m_colorBuffer->unbindAsShaderStorage(0);
    m_renderScreenProgram->unuse();

    updateWindowTitle(frameInfo);
}

void MandelbrotViewer::recreateColorBuffer(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    size_t const colorBufferBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(float4);
    if (m_colorBuffer && m_colorBuffer->getSize() == colorBufferBytes) {
        return;
    }

    if (m_colorBuffer) {
        CHECK_CUDA(cudaDeviceSynchronize());
        gfx::RHI::finish();
        m_colorBufferResource.reset();
        m_colorBuffer.reset();
    }

    m_colorBuffer = std::make_unique<gfx::Buffer>(
        gfx::Buffer::UsageFlags::ShaderStorageBuffer, colorBufferBytes, gfx::Buffer::StorageFlags::Dynamic);
    m_colorBufferResource = std::make_unique<gpu::GraphicsBufferResource>(
        *m_colorBuffer, gpu::GraphicsResource::RegisterFlags::WriteDiscard);
}

void MandelbrotViewer::updateWindowTitle(MandelbrotCompute::FrameInfo const& frameInfo) {
    std::string title =
        std::format("Zoom={:.4e}  Iter={}", static_cast<double>(frameInfo.zoom), frameInfo.maxIterations);
    if (m_playback.antialias()) {
        title += "  AA=2x2";
    }
    if (!m_playback.playing()) {
        title += "  [PAUSED]";
    }

    m_window.setTitle(title);
}

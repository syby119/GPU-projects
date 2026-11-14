#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <common/application.h>
#include <common/event/event.h>
#include <graphics/buffer.h>
#include <graphics/gl_program.h>
#include <graphics/vertex_array.h>
#include <gpu/graphics_resource/graphics_buffer_resource.h>

#include <mandelbrot_compute.h>
#include <playback_controller.h>

class MandelbrotViewer final : public Application {
public:
    struct Config {
        Application::Config applicationConfig;
        bool antialias{false};
        HighPrecisionComplex center{
            HighPrecisionFloat{"-1.416707803560595223063379502205564140068277553325999761"},
            HighPrecisionFloat{"0.000000000000000000000001192699352575212153707731000000"}
        };
        HighPrecisionFloat zoom{"1.6E22"};
    };

public:
    explicit MandelbrotViewer(Config const& config);

    ~MandelbrotViewer() override;

protected:
    void handleEvent(Event& event) override;

    void renderFrame() override;

private:
    std::unique_ptr<gfx::Buffer> m_colorBuffer;
    std::unique_ptr<gpu::GraphicsBufferResource> m_colorBufferResource;
    std::unique_ptr<MandelbrotCompute> m_mandelbrotCompute;
    std::unique_ptr<Palette> m_palette;

    std::unique_ptr<gfx::GLProgram> m_renderScreenProgram;
    std::unique_ptr<gfx::VertexArray> m_screenVertexArray;

    PlaybackController m_playback;
    HighPrecisionComplex m_startCenter{
        HighPrecisionFloat{"-1.4184000000000000000000000000000000000000"},
        HighPrecisionFloat{0}
    };
    HighPrecisionFloat m_startZoom{1035};

private:
    void recreateColorBuffer(uint32_t width, uint32_t height);

    void updateWindowTitle(MandelbrotCompute::FrameInfo const& frameInfo);
};

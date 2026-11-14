#pragma once

#include <mandelbrot.h>

// Owns the high-precision camera animation and the reference data used by the
// online renderer. The actual pixel kernel remains exposed through
// renderMandelbrotSet() in mandelbrot.h.
class MandelbrotCompute {
public:
    struct FrameInfo {
        long double zoom{1.0L};
        int maxIterations{0};
    };

public:
    MandelbrotCompute(
        int framebufferWidth,
        int framebufferHeight,
        HighPrecisionComplex const& startCenter,
        HighPrecisionFloat const& startZoom,
        HighPrecisionComplex const& finalCenter,
        HighPrecisionFloat const& finalZoom);

    MandelbrotCompute(MandelbrotCompute&&) noexcept = default;

    ~MandelbrotCompute() = default;

    MandelbrotCompute& operator=(MandelbrotCompute&&) noexcept = default;

    void setReferenceViewport(int framebufferWidth, int framebufferHeight);

    FrameInfo render(
        float4* output,
        int framebufferWidth,
        int framebufferHeight,
        double timeSeconds,
        bool antialias,
        Palette const& palette) const;

    static constexpr double durationSeconds() noexcept {
        return 150.0;
    }

private:
    HighPrecisionComplex m_startCenter;
    HighPrecisionFloat m_startZoom;
    HighPrecisionComplex m_finalCenter;
    HighPrecisionFloat m_finalZoom;
    int m_referenceWidth{0};
    int m_referenceHeight{0};

    // TODO: Add necessary data fields

private:
    // Add necessary functions here
};

#include <mandelbrot_compute.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <gpu/utility.h>

struct CameraState {
    HighPrecisionComplex center{};
    HighPrecisionFloat zoom{};
};

static CameraState cameraAtTime(
    double timeSeconds,
    HighPrecisionComplex const& startCenter,
    HighPrecisionFloat const& startZoom,
    HighPrecisionComplex const& finalCenter,
    HighPrecisionFloat const& finalZoom) {
    HighPrecisionComplex const initialCenter = startCenter;
    HighPrecisionFloat const initialZoom = startZoom;
    HighPrecisionComplex const targetCenter = finalCenter;
    HighPrecisionFloat const targetZoom = finalZoom;

    double const normalized = std::clamp(timeSeconds / MandelbrotCompute::durationSeconds(), 0.0, 1.0);
    long double const initialZoomLongDouble = startZoom.convert_to<long double>();
    long double const targetZoomLongDouble = finalZoom.convert_to<long double>();

    CameraState camera;
    if (normalized <= 0.0) {
        camera.zoom = initialZoom;
    }
    else if (normalized >= 1.0) {
        camera.zoom = targetZoom;
    }
    else {
        long double const logarithmicZoom =
            std::log10(initialZoomLongDouble) + (std::log10(targetZoomLongDouble) - std::log10(initialZoomLongDouble)) *
                                                    static_cast<long double>(normalized);
        camera.zoom = HighPrecisionFloat(std::pow(10.0L, logarithmicZoom));
    }

    double const oneMinusNormalized = 1.0 - normalized;
    HighPrecisionFloat const decay =
        (initialZoom / camera.zoom) * HighPrecisionFloat(oneMinusNormalized * oneMinusNormalized);
    camera.center = targetCenter + (initialCenter - targetCenter) * decay;

    return camera;
}

MandelbrotCompute::MandelbrotCompute(
    int framebufferWidth,
    int framebufferHeight,
    HighPrecisionComplex const& startCenter,
    HighPrecisionFloat const& startZoom,
    HighPrecisionComplex const& finalCenter,
    HighPrecisionFloat const& finalZoom)
    : m_startCenter(startCenter)
    , m_startZoom(startZoom)
    , m_finalCenter(finalCenter)
    , m_finalZoom(finalZoom) {
    m_referenceWidth = framebufferWidth;
    m_referenceHeight = framebufferHeight;
    // TODO: If additional data fields needed, initialize them here
}

void MandelbrotCompute::setReferenceViewport(int framebufferWidth, int framebufferHeight) {
    // TODO: React to the window resize event
}

MandelbrotCompute::FrameInfo MandelbrotCompute::render(
    float4* output,
    int framebufferWidth,
    int framebufferHeight,
    double timeSeconds,
    bool antialias,
    Palette const& palette) const {
    if (framebufferWidth != m_referenceWidth || framebufferHeight != m_referenceHeight) {
        throw std::logic_error("Reference data does not match framebuffer dimensions");
    }

    CameraState const camera = cameraAtTime(timeSeconds, m_startCenter, m_startZoom, m_finalCenter, m_finalZoom);

    // TODO: 1. Fill the RenderParameters according to the current frame states
    // RenderParameters parameters;
    long double const zoomLongDouble = camera.zoom.convert_to<long double>();
    double const pixcelScale =
        (HighPrecisionFloat(4) / (camera.zoom * HighPrecisionFloat(framebufferHeight))).convert_to<double>();

    // TODO: 2. Call renderMandelbrotSet to draw the Mandelbrot set into the output buffer

    return {zoomLongDouble, 0};
}

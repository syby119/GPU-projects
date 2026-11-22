#include <mandelbrot_offline_render.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <gpu/buffer/buffer.h>
#include <gpu/timer.h>
#include <gpu/utility.h>

#include <mandelbrot.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

static void selectOfflineCudaDevice() {
    int deviceCount = 0;
    cudaError_t const status = cudaGetDeviceCount(&deviceCount);
    if (status != cudaSuccess || deviceCount == 0) {
        throw std::runtime_error(
            std::format("No CUDA device is available for offline rendering: {}", cudaGetErrorString(status)));
    }

    CHECK_CUDA(cudaSetDevice(0));
    gpu::printCudaDeviceName(0);
}

static unsigned char toByte(float value) {
    return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

void renderMandelbrotSetOffline(OfflineRenderConfig const& config) {
    selectOfflineCudaDevice();

    Palette palette;

    size_t const pixelCount = static_cast<size_t>(config.width) * static_cast<size_t>(config.height);
    gpu::Buffer<float4> output(pixelCount);

    long double const zoomLongDouble = config.zoom.convert_to<long double>();
    double const pixelScale =
        (HighPrecisionFloat(4) / (config.zoom * HighPrecisionFloat(config.height))).convert_to<double>();
    if (!(zoomLongDouble > 0.0L) || !std::isfinite(zoomLongDouble) || !(pixelScale > 0.0) ||
        !std::isfinite(pixelScale)) {
        throw std::invalid_argument("Offline zoom is outside the supported FP64 CUDA range");
    }

    // TODO: 1. Fill RenderParamters according to the input parameters

    gpu::Timer timer;
    timer.tick();

    // TODO: 2. Call renderMandelbrotSet to draw the Mandelbrot set into the output buffer

    timer.tock();

    std::cout << std::format("Offline render completed in {} seconds\n", timer.getTotalTime());

    std::vector<float4> renderedPixels(pixelCount);
    CHECK_CUDA(cudaMemcpy(renderedPixels.data(), output.data(), output.sizeBytes(), cudaMemcpyDeviceToHost));

    std::vector<unsigned char> rgba(pixelCount * 4);
    for (size_t index = 0; index < pixelCount; ++index) {
        float4 const pixel = renderedPixels[index];
        rgba[4 * index + 0] = toByte(pixel.x);
        rgba[4 * index + 1] = toByte(pixel.y);
        rgba[4 * index + 2] = toByte(pixel.z);
        rgba[4 * index + 3] = toByte(pixel.w);
    }

    std::string const outputPath = config.outputPath.string();
    // CUDA row zero is bottom-left while PNG row zero is top-left.
    stbi_flip_vertically_on_write(1);
    if (stbi_write_png(outputPath.c_str(), config.width, config.height, 4, rgba.data(), config.width * 4) == 0) {
        throw std::runtime_error("Failed to write offline PNG: " + outputPath);
    }

    std::cout << "Image saved to " << config.outputPath << '\n';
}

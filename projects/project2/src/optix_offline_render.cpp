#include <optix_offline_render.h>

#include <camera_controller.h>
#include <optix_renderer.h>
#include <scene.h>

#include <gpu/buffer.h>
#include <gpu/timer.h>
#include <gpu/utility.h>
#include <optix/device_context.h>
#include <optix/utility.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

static unsigned char toByte(float value) {
    return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

static unsigned char linearToSrgbByte(float value) {
    float const linear = std::clamp(value, 0.0f, 1.0f);
    float const encoded = linear <= 0.0031308f ? 12.92f * linear : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    return toByte(encoded);
}

void renderOptixOffline(OptixOfflineConfig const& config) {
    size_t const width = config.width;
    size_t const height = config.height;
    size_t const pixelCount = width * height;

    int deviceCount = 0;
    cudaError_t const status = cudaGetDeviceCount(&deviceCount);
    if (status != cudaSuccess || deviceCount == 0) {
        throw std::runtime_error(std::string("No CUDA device is available: ") + cudaGetErrorString(status));
    }

    CHECK_CUDA(cudaSetDevice(0));
    gpu::printCudaDeviceName(0);

    CHECK_OPTIX(optixInit());

    optix::DeviceContext context;
    uint32_t const rtCoreVersion = context.getRtCoreVersion();
    if (rtCoreVersion == 0) {
        std::cout << "RT Core version: Not supported\n";
    }
    else {
        std::cout << std::format("RT Core version: {}.{}\n", rtCoreVersion / 10, rtCoreVersion % 10);
    }

    Scene scene(context, config.assetRootDir);
    OptixRenderer renderer(context, scene, config.assetRootDir / "optix/project2.optixir", config.width, config.height);
    Camera camera({12.48607f, 1.925702f, 1.317638f}, glm::quat(1, 0, 0, 0));
    CameraController cameraController({0, 3, 0});
    cameraController.lookAt(camera);
    gpu::Buffer<float4> output(pixelCount);

    gpu::Timer timer;
    timer.tick();

    renderer.render(output.data(), 0, camera, config.denoiser, config.samples);

    timer.tock();
    std::cout << std::format("Offline render completed in {} seconds\n", timer.getTotalTime());

    std::vector<float4> pixels(pixelCount);
    output.copyToHost(pixels.data());
    std::vector<unsigned char> rgba(pixelCount * 4);
    for (size_t index = 0; index < pixelCount; ++index) {
        float4 const pixel = pixels[index];
        rgba[4 * index + 0] = linearToSrgbByte(pixel.x);
        rgba[4 * index + 1] = linearToSrgbByte(pixel.y);
        rgba[4 * index + 2] = linearToSrgbByte(pixel.z);
        rgba[4 * index + 3] = toByte(pixel.w);
    }

    // OptiX image row zero is bottom-left; PNG row zero is top-left.
    stbi_flip_vertically_on_write(1);
    std::string const path = config.outputPath.string();
    if (!stbi_write_png(
            path.c_str(),
            static_cast<int>(config.width),
            static_cast<int>(config.height),
            4,
            rgba.data(),
            static_cast<int>(width * 4))) {
        throw std::runtime_error("Failed to write offline PNG: " + path);
    }
    std::cout << "Image saved to " << config.outputPath << '\n';
}

#include <mandelbrot.h>

#include <cmath>
#include <iostream>

constexpr double kEscapeRadiusSquared = 256.0;

struct SampleData {
    bool escaped;
    float smoothIteration;
};

static __device__ __forceinline__ float computeSmoothIteration(int iteration, double radiusSquared) {
    // radiusSquared is |z|^2, so 0.5 * log(radiusSquared) is log(|z|).
    return static_cast<float>(iteration) + 1.0f - log2f(0.5f * logf(static_cast<float>(radiusSquared)));
}

static __device__ __forceinline__ SampleData makeEscapedSample(int iteration, double radiusSquared) {
    return {true, computeSmoothIteration(iteration, radiusSquared)};
}

static __device__ __forceinline__ SampleData makeUnescapedSample(int maxIterations) {
    return {false, static_cast<float>(maxIterations)};
}

static __device__ __forceinline__ SampleData sampleMandelbrot(
    int x, int y, float subPixelX, float subPixelY, RenderParameters const& parameters
    /* add additional arguments if necessary */) {
    // Match the reference's FP32 sub-pixel addition before widening to FP64.
    double const fragmentX = static_cast<double>(static_cast<float>(x) + 0.5f + subPixelX);
    double const fragmentY = static_cast<double>(static_cast<float>(y) + 0.5f + subPixelY);
    Complex const planeOffset{
        (fragmentX - 0.5 * static_cast<double>(parameters.width)) * parameters.pixelScale,
        (fragmentY - 0.5 * static_cast<double>(parameters.height)) * parameters.pixelScale,
    };

    // TODO: Iterate to get the SampleData, use the following function to generate correct SampleData
    // + makeEscapedSample
    // + makeUnescapedSample

    return {true, 1};
}

static __device__ __forceinline__ float3 shade(SampleData sample, RenderParameters const& parameters) {
    if (!sample.escaped) {
        return {0.0f, 0.0f, 0.0f};
    }

    return parameters.palette.sample(sample.smoothIteration);
}

__global__ void renderMandelbrotSetKernel(float4* __restrict__ output, RenderParameters parameters
                                          /* add additional arguments if necessary */) {
    int const x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    int const y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= parameters.width || y >= parameters.height) {
        return;
    }

    float3 color = {0.0f, 0.0f, 0.0f};
    if (parameters.antialias) {
        for (int sampleY = 0; sampleY < 2; ++sampleY) {
            for (int sampleX = 0; sampleX < 2; ++sampleX) {
                float const offsetX = sampleX == 0 ? -0.25f : 0.25f;
                float const offsetY = sampleY == 0 ? -0.25f : 0.25f;
                float3 const sample = shade(sampleMandelbrot(x, y, offsetX, offsetY, parameters), parameters);
                color.x += sample.x;
                color.y += sample.y;
                color.z += sample.z;
            }
        }
        color.x *= 0.25f;
        color.y *= 0.25f;
        color.z *= 0.25f;
    }
    else {
        color = shade(sampleMandelbrot(x, y, 0.0f, 0.0f, parameters), parameters);
    }

    size_t const index = static_cast<size_t>(y) * static_cast<size_t>(parameters.width) + static_cast<size_t>(x);
    output[index] = make_float4(color.x, color.y, color.z, 1.0f);
}

void renderMandelbrotSet(float4* output, RenderParameters const& parameters) {
    // TODO: Launch renderMandelbrotSetKernel here
}

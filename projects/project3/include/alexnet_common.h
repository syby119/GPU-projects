#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>

struct NchwShape {
    int n; // Number of samples in the batch.
    int c; // Number of channels per sample.
    int h; // Height of each channel.
    int w; // Width of each channel.

    size_t size() const {
        return static_cast<size_t>(n) * c * h * w;
    }
};

struct Conv2dLayerDesc {
    int inChannels;
    int outChannels;
    int kernel;
    int stride;
    int padding;
};

struct MaxPool2dLayerDesc {
    int kernel;
    int stride;
};

struct LinearLayerDesc {
    int inFeatures;
    int outFeatures;
};

constexpr int AlexNetInputSize = 3 * 224 * 224;
constexpr int AlexNetOutputSize = 1000;
constexpr int AlexNetWorkspaceSize = 64 * 55 * 55;

// Torchvision AlexNet (inference; NCHW layout; N = batch size).
// k = kernel size, s = stride, p = padding.
//
// Layer             | Parameters     | Output shape      | Note
// ------------------+----------------+-------------------+------------------------------
// Input             |                | N x 3 x 224 x 224 |
// Conv1             | k=11, s=4, p=2 | N x 64 x 55 x 55  |
// ReLU              |                | N x 64 x 55 x 55  |
// MaxPool1          | k=3, s=2       | N x 64 x 27 x 27  |
// Conv2             | k=5, s=1, p=2  | N x 192 x 27 x 27 |
// ReLU              |                | N x 192 x 27 x 27 |
// MaxPool2          | k=3, s=2       | N x 192 x 13 x 13 |
// Conv3             | k=3, s=1, p=1  | N x 384 x 13 x 13 |
// ReLU              |                | N x 384 x 13 x 13 |
// Conv4             | k=3, s=1, p=1  | N x 256 x 13 x 13 |
// ReLU              |                | N x 256 x 13 x 13 |
// Conv5             | k=3, s=1, p=1  | N x 256 x 13 x 13 |
// ReLU              |                | N x 256 x 13 x 13 |
// MaxPool3          | k=3, s=2       | N x 256 x 6 x 6   |
// AdaptiveAvgPool2d | output=6x6     | N x 256 x 6 x 6   | Identity for 224x224; omitted
// Flatten           |                | N x 9216          |
//
// Dropout           |                | N x 9216          | Disabled during inference
// Linear1           | 9216 -> 4096   | N x 4096          |
// ReLU              |                | N x 4096          |
// Dropout           |                | N x 4096          | Disabled during inference
// Linear2           | 4096 -> 4096   | N x 4096          |
// ReLU              |                | N x 4096          |
// Linear3           | 4096 -> 1000   | N x 1000          | Raw logits
constexpr std::array<Conv2dLayerDesc, 5> AlexNetConv2dLayerDescs{{
    {.inChannels = 3, .outChannels = 64, .kernel = 11, .stride = 4, .padding = 2},
    {.inChannels = 64, .outChannels = 192, .kernel = 5, .stride = 1, .padding = 2},
    {.inChannels = 192, .outChannels = 384, .kernel = 3, .stride = 1, .padding = 1},
    {.inChannels = 384, .outChannels = 256, .kernel = 3, .stride = 1, .padding = 1},
    {.inChannels = 256, .outChannels = 256, .kernel = 3, .stride = 1, .padding = 1},
}};

constexpr MaxPool2dLayerDesc AlexNetMaxPool2dLayerDesc{.kernel = 3, .stride = 2};

constexpr std::array<LinearLayerDesc, 3> AlexNetLinearLayerDescs{{
    {.inFeatures = 256 * 6 * 6, .outFeatures = 4096},
    {.inFeatures = 4096, .outFeatures = 4096},
    {.inFeatures = 4096, .outFeatures = 1000},
}};

// Learned FP32 parameters for one convolution or linear layer, stored on the CPU.
struct LayerParameters {
    // Flattened in row-major order; the last dimension varies fastest:
    // Conv2d: [outChannels, inChannels, kernel, kernel] (OIHW).
    // Linear: [outFeatures, inFeatures].
    std::vector<float> weights;

    // One bias per output channel (Conv2d) or output feature (Linear).
    // Shared by all samples and, for Conv2d, all spatial positions.
    std::vector<float> bias;
};

// Parameters for AlexNet's five convolution layers and three linear layers.
struct AlexNetParameters {
    // Forward order: layers[0..4] = Conv1..Conv5, layers[5..7] = Linear1..Linear3.
    // ReLU, pooling and dropout have no learned parameters.
    std::array<LayerParameters, 8> layers;
};

inline size_t getBatchElementCount(int batch, size_t elementsPerImage) {
    return static_cast<size_t>(batch) * elementsPerImage;
}

inline NchwShape getConv2dOutputShape(NchwShape input, Conv2dLayerDesc desc) {
    return {
        input.n,
        desc.outChannels,
        (input.h + 2 * desc.padding - desc.kernel) / desc.stride + 1,
        (input.w + 2 * desc.padding - desc.kernel) / desc.stride + 1
    };
}

inline NchwShape getPoolOutputShape(NchwShape input, MaxPool2dLayerDesc desc) {
    return {input.n, input.c, (input.h - desc.kernel) / desc.stride + 1, (input.w - desc.kernel) / desc.stride + 1};
}

AlexNetParameters loadAlexNetParameters(std::filesystem::path const& path);

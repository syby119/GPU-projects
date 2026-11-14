#pragma once

#include "alexnet_common.h"

#include <vector>

class AlexNetCpu {
public:
    AlexNetCpu(AlexNetParameters const& parameters, int batchCapacity);

    void run(float const* input, float* logits, int batch);

private:
    AlexNetParameters const& m_parameters;
    int m_batchCapacity;

    std::vector<float> m_intermediateBuffers[2];

private:
    static NchwShape conv2d(
        float const* input,
        float const* weights,
        float const* bias,
        float* output,
        NchwShape shape,
        Conv2dLayerDesc layer);

    static void relu(float* values, size_t count);

    static NchwShape maxPool(float const* input, float* output, NchwShape shape, MaxPool2dLayerDesc layer);

    static void linear(
        float const* input,
        float const* weights,
        float const* bias,
        float* output,
        int batch,
        LinearLayerDesc layer);
};

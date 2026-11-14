#include "alexnet_cpu.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

AlexNetCpu::AlexNetCpu(AlexNetParameters const& parameters, int batchCapacity)
    : m_parameters(parameters)
    , m_batchCapacity(batchCapacity) {
    size_t const intermediateElementCount = getBatchElementCount(batchCapacity, AlexNetWorkspaceSize);

    for (auto& buffer : m_intermediateBuffers) {
        buffer.resize(intermediateElementCount);
    }
}

void AlexNetCpu::run(float const* input, float* logits, int batch) {
    if (batch <= 0 || batch > m_batchCapacity) {
        throw std::invalid_argument("Batch exceeds CPU workspace capacity");
    }

    NchwShape shape{batch, 3, 224, 224};
    int writeIndex = 0;
    float const* current = input;
    float* next = m_intermediateBuffers[writeIndex].data();

    for (size_t i = 0; i < AlexNetConv2dLayerDescs.size(); ++i) {
        auto const& params = m_parameters.layers[i];

        shape = conv2d(current, params.weights.data(), params.bias.data(), next, shape, AlexNetConv2dLayerDescs[i]);
        relu(next, shape.size());

        current = next;
        writeIndex = 1 - writeIndex;
        next = m_intermediateBuffers[writeIndex].data();

        if (i == 0 || i == 1 || i == 4) {
            shape = maxPool(current, next, shape, AlexNetMaxPool2dLayerDesc);

            current = next;
            writeIndex = 1 - writeIndex;
            next = m_intermediateBuffers[writeIndex].data();
        }
    }

    // For the fixed 224x224 input, the final pooling output is already 6x6:
    // torchvision's AdaptiveAvgPool2d(6,6) is an identity operation here.
    // Dropout is disabled during inference; the output is 1000 raw logits.
    for (size_t i = 0; i < AlexNetLinearLayerDescs.size(); ++i) {
        auto const& params = m_parameters.layers[i + AlexNetConv2dLayerDescs.size()];
        float* destination = i == 2 ? logits : next;

        linear(current, params.weights.data(), params.bias.data(), destination, batch, AlexNetLinearLayerDescs[i]);

        if (i != 2) {
            relu(destination, static_cast<size_t>(batch) * AlexNetLinearLayerDescs[i].outFeatures);
            current = destination;
            writeIndex = 1 - writeIndex;
            next = m_intermediateBuffers[writeIndex].data();
        }
    }
}

NchwShape AlexNetCpu::conv2d(
    float const* input,
    float const* weights,
    float const* bias,
    float* output,
    NchwShape shape,
    Conv2dLayerDesc layer) {
    NchwShape const out = getConv2dOutputShape(shape, layer);

    for (int n = 0; n < out.n; ++n) {
        for (int oc = 0; oc < out.c; ++oc) {
            for (int y = 0; y < out.h; ++y) {
                for (int x = 0; x < out.w; ++x) {
                    float sum = bias[oc];

                    for (int ic = 0; ic < shape.c; ++ic) {
                        for (int ky = 0; ky < layer.kernel; ++ky) {
                            int const iy = y * layer.stride - layer.padding + ky;

                            if (iy < 0 || iy >= shape.h) {
                                continue;
                            }

                            for (int kx = 0; kx < layer.kernel; ++kx) {
                                int const ix = x * layer.stride - layer.padding + kx;

                                if (ix >= 0 && ix < shape.w) {
                                    size_t const a =
                                        ((static_cast<size_t>(n) * shape.c + ic) * shape.h + iy) * shape.w + ix;
                                    size_t const b =
                                        ((static_cast<size_t>(oc) * shape.c + ic) * layer.kernel + ky) * layer.kernel +
                                        kx;

                                    sum += input[a] * weights[b];
                                }
                            }
                        }
                    }

                    output[((static_cast<size_t>(n) * out.c + oc) * out.h + y) * out.w + x] = sum;
                }
            }
        }
    }

    return out;
}

void AlexNetCpu::relu(float* values, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        values[i] = std::max(0.0f, values[i]);
    }
}

NchwShape AlexNetCpu::maxPool(float const* input, float* output, NchwShape shape, MaxPool2dLayerDesc layer) {
    NchwShape const out = getPoolOutputShape(shape, layer);

    for (int n = 0; n < out.n; ++n) {
        for (int c = 0; c < out.c; ++c) {
            for (int y = 0; y < out.h; ++y) {
                for (int x = 0; x < out.w; ++x) {
                    float maximum = -std::numeric_limits<float>::infinity();

                    for (int ky = 0; ky < layer.kernel; ++ky) {
                        for (int kx = 0; kx < layer.kernel; ++kx) {
                            size_t const index =
                                ((static_cast<size_t>(n) * shape.c + c) * shape.h + y * layer.stride + ky) * shape.w +
                                x * layer.stride + kx;
                            maximum = std::max(maximum, input[index]);
                        }
                    }

                    output[((static_cast<size_t>(n) * out.c + c) * out.h + y) * out.w + x] = maximum;
                }
            }
        }
    }

    return out;
}

void AlexNetCpu::linear(
    float const* input,
    float const* weights,
    float const* bias,
    float* output,
    int batch,
    LinearLayerDesc layer) {
    for (int n = 0; n < batch; ++n) {
        for (int oc = 0; oc < layer.outFeatures; ++oc) {
            float sum = bias[oc];

            for (int ic = 0; ic < layer.inFeatures; ++ic) {
                sum += input[static_cast<size_t>(n) * layer.inFeatures + ic] *
                       weights[static_cast<size_t>(oc) * layer.inFeatures + ic];
            }

            output[static_cast<size_t>(n) * layer.outFeatures + oc] = sum;
        }
    }
}

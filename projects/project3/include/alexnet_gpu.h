#pragma once

#include "alexnet_common.h"
#include "gpu/buffer.h"
#include "gpu/event.h"
#include "gpu/pinned_buffer.h"
#include "gpu/stream.h"

#include <array>
#include <vector>

class AlexNetGpu {
public:
    AlexNetGpu(AlexNetParameters const& parameters, int batchCapacity, int streamCount);

    ~AlexNetGpu() noexcept;

    // TODO: Add a method to run the network on the GPU.

private:
    // TODO: Add private members to store the parameters, batch capacity, stream count,
    // and any GPU resources needed for computation.
};

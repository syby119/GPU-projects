#include <cuda_runtime.h>
#include <nvml.h>

#include <common/utility.h>
#include <device_info.h>
#include <gpu/driver_utility.h>
#include <gpu/utility.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

int main() {
    println("Device Query Starting...\n");
    println(" CUDA Device Query (Runtime API, Driver API, and NVML)\n");

    FATAL_CU(cuInit(0));

    if (auto const error = nvmlInit_v2(); error != NVML_SUCCESS) {
        println("nvmlInit_v2 failed: {}", nvmlErrorString(error));
        return EXIT_FAILURE;
    }

    if (std::atexit([] { nvmlShutdown(); }) != 0) {
        println("Failed to register NVML shutdown");
        nvmlShutdown();
        return EXIT_FAILURE;
    }

    try {
        int deviceCount = 0;
        CHECK_CUDA(cudaGetDeviceCount(&deviceCount));

        if (deviceCount == 0) {
            println("There are no available device(s) that support CUDA");
            return EXIT_FAILURE;
        }

        println("Detected {} CUDA Capable device(s)\n", deviceCount);

        for (int device = 0; device < deviceCount; ++device) {
            printDeviceInfo(device);
        }

        printPeerAccessInfo(deviceCount);
    }
    catch (std::exception const& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        std::fprintf(stderr, "Unknown exception.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

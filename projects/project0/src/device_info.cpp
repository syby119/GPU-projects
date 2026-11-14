#include <device_info.h>

#include <common/utility.h>
#include <gpu/driver_utility.h>
#include <gpu/utility.h>

#include <nvml.h>

#include <cstdint>
#include <vector>

// Returns 0 when NVML cannot provide the CUDA core count.
static uint32_t getCudaCoreCount(int device) {
    char pciBusId[32];
    if (cudaDeviceGetPCIBusId(pciBusId, sizeof(pciBusId), device) != cudaSuccess) {
        return 0;
    }

    // CUDA and NVML device indices can differ; match the physical GPU by PCI bus ID.
    nvmlDevice_t nvmlDevice;
    uint32_t coreCount = 0;

    if (nvmlDeviceGetHandleByPciBusId_v2(pciBusId, &nvmlDevice) == NVML_SUCCESS) {
        if (nvmlDeviceGetNumGpuCores(nvmlDevice, &coreCount) != NVML_SUCCESS) {
            coreCount = 0;
        }
    }

    return coreCount;
}

static uint32_t convertSmVerToCudaCores(int major, int minor) {
    // This table and its fallback behavior match CUDA Samples'
    // _ConvertSMVer2Cores helper. CUDA does not expose cores-per-SM directly.
    struct SmToCores {
        uint32_t sm;
        uint32_t cores;
    };
    constexpr SmToCores kCoresPerSm[] = {
        {0x30, 192}, {0x32, 192}, {0x35, 192}, {0x37, 192}, {0x50, 128}, {0x52, 128}, {0x53, 128},
        {0x60, 64},  {0x61, 128}, {0x62, 128}, {0x70, 64},  {0x72, 64},  {0x75, 64},  {0x80, 64},
        {0x86, 128}, {0x87, 128}, {0x89, 128}, {0x90, 128}, {0xA0, 128}, {0xA1, 128}, {0xA3, 128},
        {0xA7, 128}, {0xB0, 128}, {0xC0, 128}, {0xC1, 128},
    };

    uint32_t const smVersion = (static_cast<uint32_t>(major) << 4) + static_cast<uint32_t>(minor);

    for (SmToCores const& smToCores : kCoresPerSm) {
        if (smToCores.sm == smVersion) {
            return smToCores.cores;
        }
    }

    return 0;
}

void printDeviceInfo(int device) {
    CHECK_CUDA(cudaSetDevice(device));
    cudaDeviceProp property;
    CHECK_CUDA(cudaGetDeviceProperties(&property, device));
    println("Device {}: \"{}\"", device, property.name);

    int driverVersion = 0;
    int runtimeVersion = 0;
    CHECK_CUDA(cudaDriverGetVersion(&driverVersion));
    CHECK_CUDA(cudaRuntimeGetVersion(&runtimeVersion));
    println(
        "  CUDA Driver Version / Runtime Version           {}.{} / {}.{}",
        driverVersion / 1000,
        (driverVersion % 100) / 10,
        runtimeVersion / 1000,
        (runtimeVersion % 100) / 10);
    println("  CUDA Capability Major/Minor Version Number:     {}.{}", property.major, property.minor);
    println(
        "  Device PCI Domain ID / Bus ID / Location ID:    {} / {} / {}",
        property.pciDomainID,
        property.pciBusID,
        property.pciDeviceID);

    println("  Multiprocessor Count:                           {}", property.multiProcessorCount);

    if (uint32_t totalCudaCores = getCudaCoreCount(device); totalCudaCores > 0) {
        uint32_t const coresPerSm = totalCudaCores / property.multiProcessorCount;
        println("  CUDA Cores per Multiprocessor:                  {}", coresPerSm);
        println("  Total CUDA Cores:                               {} (NVML)", totalCudaCores);
    }
    else if (uint32_t const coresPerSm = convertSmVerToCudaCores(property.major, property.minor); coresPerSm > 0) {
        uint32_t const totalCudaCores = static_cast<uint32_t>(property.multiProcessorCount) * coresPerSm;
        println("  CUDA Cores per Multiprocessor:                  {}", coresPerSm);
        println("  Total CUDA Cores:                               {} (architecture table)", totalCudaCores);
    }
    else {
        constexpr uint32_t defaultCoresPerSm = 128; // Fallback value for unknown SM versions
        uint32_t const totalCudaCores = static_cast<uint32_t>(property.multiProcessorCount) * defaultCoresPerSm;
        println("  CUDA Cores per Multiprocessor:                  {} (estimated)", defaultCoresPerSm);
        println("  Total CUDA Cores:                               {} (estimated)", totalCudaCores);
    }

    int singleToDoublePrecisionPerfRatio;
    CHECK_CUDA(
        cudaDeviceGetAttribute(&singleToDoublePrecisionPerfRatio, cudaDevAttrSingleToDoublePrecisionPerfRatio, device));
    println("  Single-to-Double Precision Performance Ratio:   {}", singleToDoublePrecisionPerfRatio);

    int clockRate;
    CHECK_CUDA(cudaDeviceGetAttribute(&clockRate, cudaDevAttrClockRate, device));
    println(
        "  GPU Max Clock Rate:                             {:.0f} MHz ({:.2f} GHz)",
        clockRate * 1e-3f,
        clockRate * 1e-6f);
    println(
        "  Total Amount of Global Memory:                  {:.0f} MBytes ({} bytes)",
        static_cast<float>(property.totalGlobalMem / 1048576.0f),
        static_cast<unsigned long long>(property.totalGlobalMem));
    int memoryClockRate;
#if CUDART_VERSION >= 13000
    CHECK_CUDA(cudaDeviceGetAttribute(&memoryClockRate, cudaDevAttrMemoryClockRate, device));
#else
    memoryClockRate = property.memoryClockRate;
#endif
    println("  Memory Clock Rate:                              {:.0f} MHz", memoryClockRate * 1e-3f);
    println("  Memory Bus Width:                               {}-bit", property.memoryBusWidth);
    println("  L2 Cache Size:                                  {} bytes", property.l2CacheSize);
    println("  Total Amount of Constant Memory:                {} bytes", property.totalConstMem);
    println("  Total Amount of Shared Memory per Block:        {} bytes", property.sharedMemPerBlock);

    int maxSharedMemoryPerBlockOptin;
    CHECK_CUDA(cudaDeviceGetAttribute(&maxSharedMemoryPerBlockOptin, cudaDevAttrMaxSharedMemoryPerBlockOptin, device));
    println("  Maximum Opt-In Shared Memory per Block:         {} bytes", maxSharedMemoryPerBlockOptin);
    println("  Total Shared Memory per Multiprocessor:         {} bytes", property.sharedMemPerMultiprocessor);
    println("  Total Number of Registers Available per Block:  {}", property.regsPerBlock);
    int maxRegistersPerMultiprocessor;
    CHECK_CUDA(
        cudaDeviceGetAttribute(&maxRegistersPerMultiprocessor, cudaDevAttrMaxRegistersPerMultiprocessor, device));
    println("  Maximum Number of Registers per Multiprocessor: {}", maxRegistersPerMultiprocessor);
#if CUDART_VERSION >= 11000
    println("  Maximum Number of Blocks per Multiprocessor:    {}", property.maxBlocksPerMultiProcessor);
#endif
    println("  Warp Size:                                      {}", property.warpSize);
    println("  Maximum Number of Threads per Multiprocessor:   {}", property.maxThreadsPerMultiProcessor);
    println("  Maximum Number of Threads per Block:            {}", property.maxThreadsPerBlock);
    println(
        "  Max Dimension Sizes of a Thread Block:          {}, {}, {}",
        property.maxThreadsDim[0],
        property.maxThreadsDim[1],
        property.maxThreadsDim[2]);
    println(
        "  Max Dimension Sizes of a Grid Size:             {}, {}, {}",
        property.maxGridSize[0],
        property.maxGridSize[1],
        property.maxGridSize[2]);

    println("  Maximum 1D Texture Dimension Size               {}", property.maxTexture1D);
    println(
        "  Maximum 2D Texture Dimension Size               {} x {}",
        property.maxTexture2D[0],
        property.maxTexture2D[1]);
    println(
        "  Maximum 3D Texture Dimension Size               {} x {} x {}",
        property.maxTexture3D[0],
        property.maxTexture3D[1],
        property.maxTexture3D[2]);
    println(
        "  Maximum Layered 1D Texture Size                 {}, {} layers",
        property.maxTexture1DLayered[0],
        property.maxTexture1DLayered[1]);
    println(
        "  Maximum Layered 2D Texture Size                 {} x {}, {} layers",
        property.maxTexture2DLayered[0],
        property.maxTexture2DLayered[1],
        property.maxTexture2DLayered[2]);
    println("  Maximum Memory Pitch:                           {} bytes", property.memPitch);
    println("  Texture Alignment:                              {} bytes", property.textureAlignment);
    println("  Surface Alignment Requirement:                  {}", property.surfaceAlignment ? "Yes" : "No");

    int gpuOverlap;
    CHECK_CUDA(cudaDeviceGetAttribute(&gpuOverlap, cudaDevAttrGpuOverlap, device));
    println(
        "  Concurrent Copy and Kernel Execution:           {} with {} Copy Engine(s)",
        gpuOverlap ? "Yes" : "No",
        property.asyncEngineCount);

    int kernelExecTimeout;
    CHECK_CUDA(cudaDeviceGetAttribute(&kernelExecTimeout, cudaDevAttrKernelExecTimeout, device));
    println("  Runtime Limit on Kernels:                       {}", kernelExecTimeout ? "Yes" : "No");

    println("  Integrated GPU Sharing Host Memory:             {}", property.integrated ? "Yes" : "No");
    println("  Supports Host Page-Locked Memory Mapping:       {}", property.canMapHostMemory ? "Yes" : "No");
    int pageableMemoryAccess;
    CHECK_CUDA(cudaDeviceGetAttribute(&pageableMemoryAccess, cudaDevAttrPageableMemoryAccess, device));
    println("  Device Supports Pageable Memory Access:         {}", pageableMemoryAccess ? "Yes" : "No");
    int hostNativeAtomicSupported;
    CHECK_CUDA(cudaDeviceGetAttribute(&hostNativeAtomicSupported, cudaDevAttrHostNativeAtomicSupported, device));
    println("  Device Supports Host Native Atomic Operations:  {}", hostNativeAtomicSupported ? "Yes" : "No");
    println("  Device Has ECC Support:                         {}", property.ECCEnabled ? "Enabled" : "Disabled");
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    println(
        "  CUDA Device Driver Mode (TCC or WDDM):          {}",
        property.tccDriver ? "TCC (Tesla Compute Cluster Driver)" : "WDDM (Windows Display Driver Model)");
#endif
    println("  Device Supports Unified Addressing (UVA):       {}", property.unifiedAddressing ? "Yes" : "No");
    println("  Device Supports Managed Memory:                 {}", property.managedMemory ? "Yes" : "No");
    int concurrentManagedAccess;
    CHECK_CUDA(cudaDeviceGetAttribute(&concurrentManagedAccess, cudaDevAttrConcurrentManagedAccess, device));
    println("  Device Supports Concurrent Managed Access:      {}", concurrentManagedAccess ? "Yes" : "No");
    println("  Device Supports Stream Priorities:              {}", property.streamPrioritiesSupported ? "Yes" : "No");
    println("  Device Supports Compute Preemption:             {}", property.computePreemptionSupported ? "Yes" : "No");
    println("  Device Supports Cooperative Kernel Launch:      {}", property.cooperativeLaunch ? "Yes" : "No");

#if CUDART_VERSION >= 11080
    int clusterLaunch;
    CHECK_CUDA(cudaDeviceGetAttribute(&clusterLaunch, cudaDevAttrClusterLaunch, device));
    println("  Device Supports Thread Block Clusters:          {}", clusterLaunch ? "Yes" : "No");
#endif

#if CUDA_VERSION >= 12000
    CUdevice driverDevice;
    CHECK_CU(cuDeviceGet(&driverDevice, device));

    int tensorMapAccess;
    CHECK_CU(cuDeviceGetAttribute(&tensorMapAccess, CU_DEVICE_ATTRIBUTE_TENSOR_MAP_ACCESS_SUPPORTED, driverDevice));
    println("  Device Supports Tensor Map Access (TMA):        {}", tensorMapAccess ? "Yes" : "No");
#endif

    char const* computeModeNames[] = {
        "Default (multiple host threads can use ::cudaSetDevice() with device simultaneously)",
        "Exclusive (only one host thread in one process is able to use ::cudaSetDevice() with this device)",
        "Prohibited (no host thread can use ::cudaSetDevice() with this device)",
        "Exclusive Process (many threads in one process is able to use ::cudaSetDevice() with this device)",
    };
    int computeMode;
    CHECK_CUDA(cudaDeviceGetAttribute(&computeMode, cudaDevAttrComputeMode, device));
    println("  Compute Mode:");
    println("     < {} >", computeModeNames[computeMode]);
    println("");
}

void printPeerAccessInfo(int deviceCount) {
    if (deviceCount < 2) {
        return;
    }

    std::vector<cudaDeviceProp> properties(deviceCount);
    std::vector<int> gpuIds;
    gpuIds.reserve(deviceCount);

    for (int device = 0; device < deviceCount; ++device) {
        CHECK_CUDA(cudaGetDeviceProperties(&properties[device], device));
        if ((properties[device].major >= 2)
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
            && properties[device].tccDriver
#endif
        ) {
            gpuIds.push_back(device);
        }
    }

    for (int srcId : gpuIds) {
        for (int dstId : gpuIds) {
            if (srcId == dstId) {
                continue;
            }

            int canAccessPeer;
            CHECK_CUDA(cudaDeviceCanAccessPeer(&canAccessPeer, srcId, dstId));
            println(
                "> Peer Access from {} (GPU{}) -> {} (GPU{}) : {}",
                properties[srcId].name,
                srcId,
                properties[dstId].name,
                dstId,
                canAccessPeer ? "Yes" : "No");
        }
    }
}

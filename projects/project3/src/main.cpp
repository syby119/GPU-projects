#include "alexnet_common.h"
#include "alexnet_cpu.h"
#include "alexnet_gpu.h"
#include "evaluation.h"
#include "image.h"

#include "common/timer.h"
#include "gpu/utility.h"

#include <cuda_profiler_api.h>
#include <nvtx3/nvToolsExt.h>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct Options {
    std::filesystem::path assets;
    std::string predictions;

    int synthetic{1024};
    int batchSize{64};
    int streamCount{2};
    int warmup{1};
    int repeat{3};
    int seed{42};
    int device{0};

    bool validation{false};
    bool quickValidation{false};
};

static void printUsage() {
    std::cout << "Project 3: AlexNet batch inference (torchvision architecture)\n"
                 "  --benchmark          Measure GPU inference on synthetic images (default)\n"
                 "  --validate           Compare CPU/GPU predictions on real images, one pass each\n"
                 "  --assets DIR         Assets root (default: assets/ beside the executable)\n"
                 "  --quick-validation   Use up to 10 images for validation (requires --validate)\n"
                 "  --predictions FILE   Export CPU/GPU predictions to CSV (validation only)\n"
                 "  --synthetic N        Number of generated RGB images (benchmark only; default: 1024)\n"
                 "  --batch-size N       Images per batch, including a partial last batch (default: 64)\n"
                 "  --streams N          CUDA streams, each with independent batch buffers (default: 2)\n"
                 "  --warmup N           Untimed complete GPU passes (benchmark only; default: 1)\n"
                 "  --repeat N           Timed complete GPU passes (benchmark only; default: 3)\n"
                 "  --seed N             Nonnegative random seed (benchmark only; default: 42)\n"
                 "  --device N           Nonnegative CUDA device index (default: 0)\n"
                 "  --help, -h           Show this help\n"
                 "GPU end-to-end time includes CPU crop/normalize/layout, H2D, full forward, D2H,\n"
                 "and completed CPU outputs. Model parameters and workspaces are prepared outside timing.\n"
                 "Synthetic images are generated before benchmark timing.\n"
                 "Model parameters: models/alexnet.bin, relative to --assets.\n"
                 "Validation data: dataset/images.txt, labels.txt and classes.txt.\n"
                 "Validation fails if GPU Top-1 or Top-5 sets differ from CPU.\n";
}

static int parseInt(std::string_view text, std::string_view name, bool allowZero = false) {
    int value = 0;
    auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);

    if (error != std::errc{} || end != text.data() + text.size() || value < (allowZero ? 0 : 1)) {
        throw std::invalid_argument(
            std::string(name) + " requires a " + (allowZero ? "nonnegative" : "positive") + " integer");
    }

    return value;
}

static Options parse(int argc, char** argv) {
    Options options{.assets = std::filesystem::absolute(argv[0]).parent_path() / "assets"};
    bool benchmarkSpecified = false;
    bool benchmarkOptionsSpecified = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view const name = argv[i];

        if (name == "--benchmark") {
            benchmarkSpecified = true;
            continue;
        }
        if (name == "--validate") {
            options.validation = true;
            continue;
        }
        if (name == "--quick-validation") {
            options.quickValidation = true;
            continue;
        }
        if (i + 1 >= argc) {
            throw std::invalid_argument("Missing value for " + std::string(name));
        }

        std::string_view const value = argv[++i];

        if (name == "--assets") {
            options.assets = value;
        }
        else if (name == "--predictions") {
            options.predictions = value;
        }
        else if (name == "--synthetic") {
            options.synthetic = parseInt(value, name);
            benchmarkOptionsSpecified = true;
        }
        else if (name == "--batch-size") {
            options.batchSize = parseInt(value, name);
        }
        else if (name == "--streams") {
            options.streamCount = parseInt(value, name);
        }
        else if (name == "--repeat") {
            options.repeat = parseInt(value, name);
            benchmarkOptionsSpecified = true;
        }
        else if (name == "--warmup") {
            options.warmup = parseInt(value, name, true);
            benchmarkOptionsSpecified = true;
        }
        else if (name == "--seed") {
            options.seed = parseInt(value, name, true);
            benchmarkOptionsSpecified = true;
        }
        else if (name == "--device") {
            options.device = parseInt(value, name, true);
        }
        else {
            throw std::invalid_argument("Unknown option: " + std::string(name));
        }
    }

    if (options.assets.empty()) {
        throw std::invalid_argument("--assets must not be empty");
    }
    if (benchmarkSpecified && options.validation) {
        throw std::invalid_argument("Choose --benchmark or --validate");
    }
    if (options.validation) {
        if (benchmarkOptionsSpecified) {
            throw std::invalid_argument("--synthetic, --warmup, --repeat and --seed are only supported by --benchmark");
        }
    }
    else if (options.quickValidation || !options.predictions.empty()) {
        throw std::invalid_argument("--quick-validation and --predictions require --validate");
    }

    return options;
}

static void runGpuInference(AlexNetGpu& alexNet, std::vector<Image> const& images, int batchSize, float* results) {
    nvtxRangePushA("GPU inference pass");

    // TODO: Implement GPU inference using AlexNetGpu. The following is a placeholder for the actual implementation.

    nvtxRangePop();
}

static bool validate(Options const& options, AlexNetParameters const& parameters, std::vector<Image> const& images) {
    auto const datasetDirectory = options.assets / "dataset";
    auto const labels =
        readLabels(datasetDirectory / (options.quickValidation ? "cpu-check-labels.txt" : "labels.txt"), images.size());
    auto const classNames = readClassNames(datasetDirectory / "classes.txt");

    AlexNetCpu alexNetCpu(parameters, options.batchSize);
    AlexNetGpu alexNetGpu(parameters, options.batchSize, options.streamCount);
    std::vector<float> input(getBatchElementCount(options.batchSize, AlexNetInputSize));

    size_t const resultCount = images.size() * AlexNetOutputSize;
    std::vector<float> cpuResults(resultCount, std::numeric_limits<float>::quiet_NaN());
    std::vector<float> gpuResults(resultCount, std::numeric_limits<float>::quiet_NaN());

    std::cout << std::format(
        "Validation: images={} batch_size={} streams={}\nRunning CPU inference...\n",
        images.size(),
        options.batchSize,
        options.streamCount);

    nvtxRangePushA("CPU reference pass");
    for (size_t first = 0; first < images.size(); first += options.batchSize) {
        int const count = static_cast<int>(std::min(static_cast<size_t>(options.batchSize), images.size() - first));
        prepareBatch(images, first, count, input.data());
        alexNetCpu.run(input.data(), cpuResults.data() + first * AlexNetOutputSize, count);
    }
    nvtxRangePop();

    std::cout << "Running GPU inference...\n";

    runGpuInference(alexNetGpu, images, options.batchSize, gpuResults.data());

    auto const cpuPredictions = getBatchTop5Classes(cpuResults);
    auto const gpuPredictions = getBatchTop5Classes(gpuResults);

    auto printAccuracy = [](char const* name, ClassificationAccuracy const& accuracy) {
        std::cout << std::format(
            "{} ground-truth accuracy: top-1={:.2f}% ({}/{}), top-5={:.2f}% ({}/{})\n",
            name,
            100.0 * accuracy.top1Correct / accuracy.images,
            accuracy.top1Correct,
            accuracy.images,
            100.0 * accuracy.top5Correct / accuracy.images,
            accuracy.top5Correct,
            accuracy.images);
    };

    printAccuracy("CPU", getClassificationAccuracy(cpuPredictions, labels));
    printAccuracy("GPU", getClassificationAccuracy(gpuPredictions, labels));

    auto const agreement = getClassificationAgreement(cpuPredictions, gpuPredictions);
    std::cout << std::format(
        "CPU/GPU prediction agreement: top-1={:.2f}% ({}/{}), top-5 set={:.2f}% ({}/{})\n",
        100.0 * agreement.top1Matches / agreement.images,
        agreement.top1Matches,
        agreement.images,
        100.0 * agreement.top5SetMatches / agreement.images,
        agreement.top5SetMatches,
        agreement.images);

    if (!options.predictions.empty()) {
        writePredictions(options.predictions, cpuPredictions, gpuPredictions, labels, classNames);
    }

    bool const passed = agreement.top1Matches == images.size() && agreement.top5SetMatches == images.size();
    std::cout << std::format("Validation: {}\n", passed ? "PASSED" : "FAILED");
    return passed;
}

static void benchmark(Options const& options, AlexNetParameters const& parameters, std::vector<Image> const& images) {
    AlexNetGpu alexNet(parameters, options.batchSize, options.streamCount);
    std::vector<float> results(images.size() * AlexNetOutputSize);

    std::cout << std::format(
        "Benchmark: synthetic_images={} seed={} batch_size={} streams={} warmup={} repeat={}\n"
        "Timing: CPU preparation + H2D + GPU inference + D2H + completed CPU outputs\n",
        images.size(),
        options.seed,
        options.batchSize,
        options.streamCount,
        options.warmup,
        options.repeat);

    for (int i = 0; i < options.warmup; ++i) {
        runGpuInference(alexNet, images, options.batchSize, results.data());
    }

    CHECK_CUDA(cudaProfilerStart());

    Timer timer;
    timer.tick();

    for (int i = 0; i < options.repeat; ++i) {
        runGpuInference(alexNet, images, options.batchSize, results.data());
    }

    double const averageMs = timer.tock() * 1000.0 / options.repeat;

    CHECK_CUDA(cudaProfilerStop());

    std::cout << std::format("GPU end-to-end ms (average): {:.4f}\n", averageMs);
    std::cout << std::format("GPU throughput: {:.4f} images/s\n", images.size() * 1000.0 / averageMs);
}

int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::string_view(argv[i]) == "--help" || std::string_view(argv[i]) == "-h") {
                printUsage();
                return EXIT_SUCCESS;
            }
        }

        Options const options = parse(argc, argv);
        CHECK_CUDA(cudaSetDevice(options.device));
        CHECK_CUDA(cudaFree(nullptr));

        gpu::printCudaDeviceName(options.device);

        AlexNetParameters const parameters = loadAlexNetParameters(options.assets / "models" / "alexnet.bin");

        if (options.validation) {
            auto const imageListPath =
                options.assets / "dataset" / (options.quickValidation ? "cpu-check-images.txt" : "images.txt");
            auto const images = readImageList(imageListPath);
            if (!validate(options, parameters, images)) {
                return EXIT_FAILURE;
            }
        }
        else {
            auto const images = makeSyntheticImages(options.synthetic, options.seed);
            benchmark(options, parameters, images);
        }
    }
    catch (std::exception const& error) {
        std::cerr << std::format("project3: {}\n", error.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

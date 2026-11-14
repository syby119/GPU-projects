#include "evaluation.h"

#include "alexnet_common.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>

std::vector<int> readLabels(std::filesystem::path const& path, size_t imageCount) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Cannot open ground-truth labels: " + path.string());
    }

    std::vector<int> labels;
    std::string line;

    while (std::getline(file, line)) {
        auto const first = line.find_first_not_of(" \t\r");

        if (first == std::string::npos || line[first] == '#') {
            continue;
        }

        std::istringstream fields(line);
        int label = -1;
        std::string extra;

        if (!(fields >> label) || fields >> extra || label < 0 || label >= AlexNetOutputSize) {
            throw std::runtime_error("Ground-truth labels must be ImageNet-1K indices in [0,999]");
        }

        labels.push_back(label);
    }

    if (labels.size() != imageCount) {
        throw std::runtime_error("Ground-truth label count must equal image count");
    }

    return labels;
}

std::vector<std::string> readClassNames(std::filesystem::path const& path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Cannot open class names: " + path.string());
    }

    std::vector<std::string> names;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            throw std::runtime_error("Empty class name");
        }

        names.push_back(line);
    }

    if (names.size() != AlexNetOutputSize) {
        throw std::runtime_error("Class-name file must contain all 1000 ImageNet classes in output order");
    }

    return names;
}

Top5Classes getTop5Classes(std::span<float const> logits) {
    if (logits.size() != AlexNetOutputSize ||
        !std::all_of(logits.begin(), logits.end(), [](float value) { return std::isfinite(value); })) {
        throw std::runtime_error("Expected 1000 finite classification scores");
    }

    std::array<int, AlexNetOutputSize> indices{};
    std::iota(indices.begin(), indices.end(), 0);

    std::partial_sort(indices.begin(), indices.begin() + 5, indices.end(), [&](int a, int b) {
        return logits[a] != logits[b] ? logits[a] > logits[b] : a < b;
    });

    return {indices[0], indices[1], indices[2], indices[3], indices[4]};
}

std::vector<Top5Classes> getBatchTop5Classes(std::span<float const> logits) {
    if (logits.empty() || logits.size() % AlexNetOutputSize != 0) {
        throw std::runtime_error("Invalid classification output size");
    }

    std::vector<Top5Classes> result;
    result.reserve(logits.size() / AlexNetOutputSize);

    for (size_t i = 0; i < logits.size(); i += AlexNetOutputSize) {
        result.push_back(getTop5Classes(logits.subspan(i, AlexNetOutputSize)));
    }

    return result;
}

ClassificationAccuracy
getClassificationAccuracy(std::span<Top5Classes const> predictions, std::span<int const> labels) {
    if (labels.empty() || predictions.size() != labels.size()) {
        throw std::runtime_error("Invalid predictions or labels for classification accuracy");
    }

    ClassificationAccuracy result{labels.size()};

    for (size_t i = 0; i < labels.size(); ++i) {
        if (labels[i] < 0 || labels[i] >= AlexNetOutputSize) {
            throw std::runtime_error("Ground-truth class index is outside [0,999]");
        }

        auto const& top = predictions[i];
        result.top1Correct += top[0] == labels[i];
        result.top5Correct += std::find(top.begin(), top.end(), labels[i]) != top.end();
    }

    return result;
}

ClassificationAgreement
getClassificationAgreement(std::span<Top5Classes const> expected, std::span<Top5Classes const> actual) {
    if (expected.empty() || expected.size() != actual.size()) {
        throw std::runtime_error("Invalid prediction arrays for classification comparison");
    }

    ClassificationAgreement result{expected.size()};

    for (size_t i = 0; i < result.images; ++i) {
        auto a = expected[i];
        auto b = actual[i];
        result.top1Matches += a[0] == b[0];
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        result.top5SetMatches += a == b;
    }

    return result;
}

std::string classList(std::array<int, 5> const& classes) {
    std::string result;

    for (int value : classes) {
        if (!result.empty()) {
            result += ' ';
        }
        result += std::to_string(value);
    }

    return result;
}

std::string csvString(std::string const& value) {
    std::string result = "\"";

    for (char character : value) {
        if (character == '"') {
            result += '"';
        }
        result += character;
    }

    return result + '"';
}

void writePredictions(
    std::filesystem::path const& path,
    std::span<Top5Classes const> cpuPredictions,
    std::span<Top5Classes const> gpuPredictions,
    std::span<int const> labels,
    std::span<std::string const> classNames) {
    if (cpuPredictions.empty() || gpuPredictions.size() != cpuPredictions.size() ||
        (!labels.empty() && labels.size() != cpuPredictions.size()) ||
        (!classNames.empty() && classNames.size() != AlexNetOutputSize)) {
        throw std::runtime_error("Invalid prediction export data");
    }

    std::ofstream file(path);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file << "image_index,ground_truth,cpu_top1,cpu_class,cpu_top5,gpu_top1,gpu_class,gpu_top5,"
            "cpu_top1_correct,cpu_top5_correct,gpu_top1_correct,gpu_top5_correct\n";

    auto writePrediction = [&](Top5Classes const& top) {
        file << std::format(
            ",{},{},{}", top[0], csvString(classNames.empty() ? "" : classNames[top[0]]), csvString(classList(top)));
    };

    for (size_t i = 0; i < cpuPredictions.size(); ++i) {
        auto const& cpuPrediction = cpuPredictions[i];
        auto const& gpuPrediction = gpuPredictions[i];
        file << std::format("{},", i);

        if (!labels.empty()) {
            file << labels[i];
        }
        writePrediction(cpuPrediction);
        writePrediction(gpuPrediction);

        if (!labels.empty()) {
            file << std::format(
                ",{},{},{},{}",
                static_cast<int>(cpuPrediction[0] == labels[i]),
                static_cast<int>(
                    std::find(cpuPrediction.begin(), cpuPrediction.end(), labels[i]) != cpuPrediction.end()),
                static_cast<int>(gpuPrediction[0] == labels[i]),
                static_cast<int>(
                    std::find(gpuPrediction.begin(), gpuPrediction.end(), labels[i]) != gpuPrediction.end()));
        }
        else {
            file << ",,,,";
        }
        file << '\n';
    }

    file.flush();
}

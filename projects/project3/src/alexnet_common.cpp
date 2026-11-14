#include "alexnet_common.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

static void readWeightBytes(std::ifstream& file, void* destination, size_t bytes) {
    if (!file.read(static_cast<char*>(destination), static_cast<std::streamsize>(bytes))) {
        throw std::runtime_error("Truncated AlexNet parameter file");
    }
}

template <typename T>
static T readWeightValue(std::ifstream& file) {
    T value{};
    readWeightBytes(file, &value, sizeof(value));

    return value;
}

static std::vector<float>
readWeightTensor(std::ifstream& file, std::string const& expectedName, std::vector<uint32_t> const& expectedShape) {
    uint32_t const nameLength = readWeightValue<uint32_t>(file);

    if (nameLength != expectedName.size()) {
        throw std::runtime_error("Unexpected weight tensor; expected " + expectedName);
    }

    std::string name(nameLength, '\0');
    readWeightBytes(file, name.data(), name.size());

    if (name != expectedName || readWeightValue<uint32_t>(file) != 1 ||
        readWeightValue<uint32_t>(file) != expectedShape.size()) {
        throw std::runtime_error("Wrong tensor name, dtype, or rank for " + expectedName);
    }

    size_t count = 1;

    for (uint32_t const expected : expectedShape) {
        if (readWeightValue<uint32_t>(file) != expected) {
            throw std::runtime_error("Wrong tensor dimensions for " + expectedName);
        }

        count *= expected;
    }

    if (readWeightValue<uint64_t>(file) != count * sizeof(float)) {
        throw std::runtime_error("Wrong tensor byte count for " + expectedName);
    }

    std::vector<float> values(count);
    readWeightBytes(file, values.data(), values.size() * sizeof(float));

    for (float const value : values) {
        if (!std::isfinite(value)) {
            throw std::runtime_error("Nonfinite weight in " + expectedName);
        }
    }

    return values;
}

AlexNetParameters loadAlexNetParameters(std::filesystem::path const& path) {
    static_assert(std::endian::native == std::endian::little);
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(
            "Cannot open parameters: " + path.string() +
            "; run scripts/prepare_weights.py to convert the official checkpoint");
    }

    std::array<char, 8> magic{};
    readWeightBytes(file, magic.data(), magic.size());

    if (magic != std::array<char, 8>{'A', 'L', 'E', 'X', 'N', 'E', 'T', '1'} || readWeightValue<uint32_t>(file) != 1 ||
        readWeightValue<uint32_t>(file) != 16) {
        throw std::runtime_error("Not a supported AlexNet binary parameter file");
    }

    std::array<char const*, 8> const names{
        "features.0",
        "features.3",
        "features.6",
        "features.8",
        "features.10",
        "classifier.1",
        "classifier.4",
        "classifier.6"
    };

    AlexNetParameters result;

    for (size_t i = 0; i < result.layers.size(); ++i) {
        std::vector<uint32_t> shape;

        if (i < AlexNetConv2dLayerDescs.size()) {
            auto const layer = AlexNetConv2dLayerDescs[i];
            shape = {
                static_cast<uint32_t>(layer.outChannels),
                static_cast<uint32_t>(layer.inChannels),
                static_cast<uint32_t>(layer.kernel),
                static_cast<uint32_t>(layer.kernel)
            };
        }
        else {
            auto const layer = AlexNetLinearLayerDescs[i - AlexNetConv2dLayerDescs.size()];
            shape = {static_cast<uint32_t>(layer.outFeatures), static_cast<uint32_t>(layer.inFeatures)};
        }

        result.layers[i].weights = readWeightTensor(file, std::string(names[i]) + ".weight", shape);
        result.layers[i].bias = readWeightTensor(file, std::string(names[i]) + ".bias", {shape[0]});
    }

    if (file.peek() != std::ifstream::traits_type::eof()) {
        throw std::runtime_error("Unexpected trailing data in AlexNet parameter file");
    }

    return result;
}

#include "image.h"

#include "alexnet_common.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

static std::string ppmToken(std::ifstream& file) {
    std::string result;
    char value = '\0';

    while (file.get(value)) {
        if (value == '#') {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else if (!std::isspace(static_cast<unsigned char>(value))) {
            result += value;
            break;
        }
    }

    while (file.get(value)) {
        if (std::isspace(static_cast<unsigned char>(value))) {
            if (value == '\r' && file.peek() == '\n') {
                file.get();
            }
            return result;
        }
        result += value;
    }

    throw std::runtime_error("Incomplete PPM header");
}

static int ppmInteger(std::ifstream& file) {
    std::string const token = ppmToken(file);
    int value = 0;
    auto const [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);

    if (error != std::errc{} || end != token.data() + token.size() || value <= 0) {
        throw std::runtime_error("Invalid PPM dimension or maximum value");
    }

    return value;
}

Image readImage(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file || ppmToken(file) != "P6") {
        throw std::runtime_error("Expected an RGB P6 PPM image: " + path.string());
    }

    int const width = ppmInteger(file);
    int const height = ppmInteger(file);
    int const maximum = ppmInteger(file);

    if (maximum != 255 || std::min(width, height) != 256 ||
        static_cast<size_t>(width) > std::numeric_limits<size_t>::max() / 3 / height) {
        throw std::runtime_error("PPM must have maxval=255 and its shorter side resized to 256: " + path.string());
    }

    size_t const byteCount = static_cast<size_t>(width) * height * 3;
    Image image{std::make_unique_for_overwrite<uint8_t[]>(byteCount), width, height};

    if (!file.read(reinterpret_cast<char*>(image.pixels.get()), static_cast<std::streamsize>(byteCount))) {
        throw std::runtime_error("Truncated PPM pixels: " + path.string());
    }

    return image;
}

std::vector<Image> readImageList(std::filesystem::path const& path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Cannot open image list: " + path.string());
    }

    std::vector<Image> images;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (!line.empty() && line.front() != '#') {
            images.push_back(readImage(path.parent_path() / std::filesystem::path(line)));
        }
    }

    if (images.empty()) {
        throw std::runtime_error("Image list is empty");
    }

    return images;
}

std::vector<Image> makeSyntheticImages(int count, unsigned seed) {
    assert(count > 0);

    std::mt19937 random(seed);
    std::vector<Image> images;
    constexpr size_t byteCount = 256 * 256 * 3;

    for (int i = 0; i < count; ++i) {
        Image image{std::make_unique_for_overwrite<uint8_t[]>(byteCount), 256, 256};

        for (size_t j = 0; j < byteCount; ++j) {
            image.pixels[j] = static_cast<uint8_t>(random() & 255);
        }

        images.push_back(std::move(image));
    }

    return images;
}

void prepareBatch(std::vector<Image> const& images, size_t first, int count, float* destination) {
    assert(count > 0);
    assert(first + count <= images.size());

    std::array<float, 3> const mean{0.485f, 0.456f, 0.406f};
    std::array<float, 3> const deviation{0.229f, 0.224f, 0.225f};

    for (int n = 0; n < count; ++n) {
        auto const& image = images[first + n];
        // torchvision center_crop uses round-to-nearest-even on half offsets.
        int const left = static_cast<int>(std::nearbyint((image.width - 224) / 2.0));
        int const top = static_cast<int>(std::nearbyint((image.height - 224) / 2.0));

        for (int channel = 0; channel < 3; ++channel) {
            for (int y = 0; y < 224; ++y) {
                for (int x = 0; x < 224; ++x) {
                    size_t const source = (static_cast<size_t>(top + y) * image.width + left + x) * 3 + channel;
                    size_t const target = static_cast<size_t>(n) * AlexNetInputSize + (channel * 224 + y) * 224 + x;

                    destination[target] = (image.pixels[source] / 255.0f - mean[channel]) / deviation[channel];
                }
            }
        }
    }
}

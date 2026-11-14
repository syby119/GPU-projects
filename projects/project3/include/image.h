#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

struct Image {
    std::unique_ptr<uint8_t[]> pixels; // Interleaved RGB bytes.
    int width;
    int height;
};

// Read one image into interleaved RGB bytes.
// Currently supports only binary P6 PPM with maxval=255 and its shorter side resized to 256 pixels.
Image readImage(std::filesystem::path const& path);

// Read all images from a text file containing one PPM path per line.
// Relative paths are resolved against the list file's directory.
// Empty lines and lines beginning with '#' are ignored.
std::vector<Image> readImageList(std::filesystem::path const& path);

// Generate count RGB noise images of size 256 x 256 for GPU benchmarks.
// The same seed produces the same pixel data.
std::vector<Image> makeSyntheticImages(int count, unsigned seed);

// Source images have their shorter side pre-resized to 256 pixels.
// Each invocation performs center crop, byte-to-float conversion, HWC-to-CHW,
// and torchvision's per-channel normalization directly into the destination.
void prepareBatch(std::vector<Image> const& images, size_t first, int count, float* destination);

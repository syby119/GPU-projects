#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

// Five ImageNet class indices in [0, 999], ordered by descending score.
using Top5Classes = std::array<int, 5>;

// Classification counts measured against ground-truth labels.
struct ClassificationAccuracy {
    size_t images{0};      // Total number of evaluated images.
    size_t top1Correct{0}; // Images whose first predicted class matches the label.
    size_t top5Correct{0}; // Images whose five predicted classes include the label.
};

// Agreement counts between CPU and GPU prediction arrays.
struct ClassificationAgreement {
    size_t images{0};         // Total number of compared images.
    size_t top1Matches{0};    // Images with the same first predicted class.
    size_t top5SetMatches{0}; // Images with identical Top-5 class sets, ignoring order.
};

// Read exactly imageCount ground-truth labels, one class index [0, 999] per data line.
// Blank lines and comment lines beginning with '#' are ignored.
// File order must match the image-list order.
std::vector<int> readLabels(std::filesystem::path const& path, size_t imageCount);

// Read exactly 1000 nonempty class names in model output order.
// The first line names class 0; each subsequent line names the next class.
std::vector<std::string> readClassNames(std::filesystem::path const& path);

// Select the five highest-scoring classes from one image's 1000 finite logits.
// Return class indices in descending score order, breaking ties by increasing class index.
Top5Classes getTop5Classes(std::span<float const> logits);

// Apply getTop5Classes to consecutive groups of 1000 logits, one group per image.
// Input must be nonempty and contain complete groups.
// Return one Top5Classes per image, preserving input order.
std::vector<Top5Classes> getBatchTop5Classes(std::span<float const> logits);

// Compare predictions with ground-truth labels and return Top-1/Top-5 correct counts.
// Inputs must be nonempty, have equal lengths, and refer to the same images in the same order.
// Each label must be a class index in [0, 999].
ClassificationAccuracy getClassificationAccuracy(std::span<Top5Classes const> predictions, std::span<int const> labels);

// Compare two prediction arrays and return Top-1 and Top-5 set match counts.
// Inputs must be nonempty, have equal lengths, and refer to the same images in the same order.
// Top-5 comparisons require identical class sets; their internal order is ignored.
ClassificationAgreement
getClassificationAgreement(std::span<Top5Classes const> expected, std::span<Top5Classes const> actual);

// Write one CSV row per image with CPU/GPU predictions and correctness against ground truth.
// All prediction arrays must be nonempty, have equal lengths, and use the same image order.
// labels and classNames are optional. Labels must have the same image count when provided;
// classNames must contain all 1000 names in class-index order. Missing information is left blank.
void writePredictions(
    std::filesystem::path const& path,
    std::span<Top5Classes const> cpuPredictions,
    std::span<Top5Classes const> gpuPredictions,
    std::span<int const> labels,
    std::span<std::string const> classNames);

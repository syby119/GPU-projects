#pragma once

#include <cstddef>
#include <limits>
#include <span>

namespace gpu::detail {

template <typename T, size_t Extent>
struct BufferStorage {
    static_assert(Extent <= std::numeric_limits<size_t>::max() / sizeof(T), "CUDA buffer size overflows size_t");

    T* data{};

    // A moved-from or released owner has no allocation.
    constexpr size_t size() const noexcept {
        return data ? Extent : 0;
    }
};

template <typename T>
struct BufferStorage<T, std::dynamic_extent> {
    T* data{};
    size_t count{};

    constexpr size_t size() const noexcept {
        return count;
    }
};

} // namespace gpu::detail

#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

#include "gpu/buffer/buffer_storage.h"
#include "gpu/utility.h"

namespace gpu {

template <typename T, size_t Extent = std::dynamic_extent>
class Buffer {
    using Storage = detail::BufferStorage<T, Extent>;

public:
    static constexpr size_t extent = Extent;

    Buffer() noexcept
        requires(Extent == std::dynamic_extent)
    = default;

    Buffer() noexcept(Extent == 0)
        requires(Extent != std::dynamic_extent)
        : m_storage{allocate(Extent)} {}

    explicit Buffer(size_t count)
        requires(Extent == std::dynamic_extent)
        : m_storage{allocate(count), count} {}

    Buffer(Buffer&& other) noexcept
        : m_storage{std::exchange(other.m_storage, {})} {}

    ~Buffer() noexcept {
        if (m_storage.data) {
            LOG_CUDA(cudaFree(m_storage.data));
        }
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            Buffer temporary(std::move(other));
            swap(temporary);
        }

        return *this;
    }

    explicit operator bool() const noexcept {
        return m_storage.data != nullptr;
    }

    T* data() noexcept {
        return m_storage.data;
    }

    T const* data() const noexcept {
        return m_storage.data;
    }

    constexpr size_t size() const noexcept {
        return m_storage.size();
    }

    constexpr size_t sizeBytes() const noexcept {
        return size() * sizeof(T);
    }

    constexpr bool empty() const noexcept {
        return size() == 0;
    }

    void reallocate(size_t count)
        requires(Extent == std::dynamic_extent)
    {
        if (count != size()) {
            Buffer temporary(count);
            swap(temporary);
        }
    }

    void copyFromHost(T const* src) {
        CHECK_CUDA(cudaMemcpy(data(), src, sizeBytes(), cudaMemcpyHostToDevice));
    }

    void copyFromHost(std::span<T const> src) {
        CHECK_CUDA(cudaMemcpy(data(), src.data(), src.size_bytes(), cudaMemcpyHostToDevice));
    }

    void copyToHost(T* dst) {
        CHECK_CUDA(cudaMemcpy(dst, data(), sizeBytes(), cudaMemcpyDeviceToHost));
    }

    void copyToHost(std::span<T> dst) {
        CHECK_CUDA(cudaMemcpy(dst.data(), data(), dst.size_bytes(), cudaMemcpyDeviceToHost));
    }

    static Buffer adopt(T* dptr, size_t count) noexcept
        requires(Extent == std::dynamic_extent)
    {
        return Buffer{Storage{dptr, count}};
    }

    static Buffer adopt(T* dptr) noexcept
        requires(Extent != std::dynamic_extent)
    {
        return Buffer{Storage{dptr}};
    }

    T* release() noexcept {
        return std::exchange(m_storage, {}).data;
    }

    void swap(Buffer& other) noexcept {
        std::swap(m_storage, other.m_storage);
    }

    friend void swap(Buffer& a, Buffer& b) noexcept {
        a.swap(b);
    }

private:
    explicit Buffer(Storage storage) noexcept
        : m_storage{storage} {}

    static T* allocate(size_t count) {
        if (count == 0) {
            return nullptr;
        }

        if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::length_error("CUDA buffer size overflows size_t");
        }

        T* dptr{};
        CHECK_CUDA(cudaMalloc(&dptr, count * sizeof(T)));
        return dptr;
    }

private:
    Storage m_storage{};
};

} // namespace gpu

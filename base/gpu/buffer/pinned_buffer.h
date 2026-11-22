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
class PinnedBuffer {
    using Storage = detail::BufferStorage<T, Extent>;

public:
    static constexpr size_t extent = Extent;

    PinnedBuffer() noexcept
        requires(Extent == std::dynamic_extent)
    = default;

    PinnedBuffer() noexcept(Extent == 0)
        requires(Extent != std::dynamic_extent)
        : m_storage{allocate(Extent)} {}

    explicit PinnedBuffer(size_t count)
        requires(Extent == std::dynamic_extent)
        : m_storage{allocate(count), count} {}

    PinnedBuffer(PinnedBuffer&& other) noexcept
        : m_storage{std::exchange(other.m_storage, {})} {}

    ~PinnedBuffer() noexcept {
        if (m_storage.data) {
            LOG_CUDA(cudaFreeHost(m_storage.data));
        }
    }

    PinnedBuffer& operator=(PinnedBuffer&& other) noexcept {
        if (this != &other) {
            PinnedBuffer temporary(std::move(other));
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
            PinnedBuffer temporary(count);
            swap(temporary);
        }
    }

    void copyFromHost(T const* src) {
        CHECK_CUDA(cudaMemcpy(data(), src, sizeBytes(), cudaMemcpyHostToHost));
    }

    void copyFromHost(std::span<T const> src) {
        CHECK_CUDA(cudaMemcpy(data(), src.data(), src.size_bytes(), cudaMemcpyHostToHost));
    }

    void copyToHost(T* dst) {
        CHECK_CUDA(cudaMemcpy(dst, data(), sizeBytes(), cudaMemcpyHostToHost));
    }

    void copyToHost(std::span<T> dst) {
        CHECK_CUDA(cudaMemcpy(dst.data(), data(), dst.size_bytes(), cudaMemcpyHostToHost));
    }

    // Keep both buffers alive until work in the stream has completed.
    void copyFromDeviceAsync(T const* src, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(data(), src, sizeBytes(), cudaMemcpyDeviceToHost, stream));
    }

    void copyFromDeviceAsync(std::span<T const> src, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(data(), src.data(), src.size_bytes(), cudaMemcpyDeviceToHost, stream));
    }

    void copyToDeviceAsync(T* dst, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(dst, data(), sizeBytes(), cudaMemcpyHostToDevice, stream));
    }

    void copyToDeviceAsync(std::span<T> dst, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(dst.data(), data(), dst.size_bytes(), cudaMemcpyHostToDevice, stream));
    }

    static PinnedBuffer adopt(T* hptr, size_t count) noexcept
        requires(Extent == std::dynamic_extent)
    {
        return PinnedBuffer{Storage{hptr, count}};
    }

    static PinnedBuffer adopt(T* hptr) noexcept
        requires(Extent != std::dynamic_extent)
    {
        return PinnedBuffer{Storage{hptr}};
    }

    T* release() noexcept {
        return std::exchange(m_storage, {}).data;
    }

    void swap(PinnedBuffer& other) noexcept {
        std::swap(m_storage, other.m_storage);
    }

    friend void swap(PinnedBuffer& a, PinnedBuffer& b) noexcept {
        a.swap(b);
    }

private:
    explicit PinnedBuffer(Storage storage) noexcept
        : m_storage{storage} {}

    static T* allocate(size_t count) {
        if (count == 0) {
            return nullptr;
        }

        if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::length_error("CUDA pinned buffer size overflows size_t");
        }

        T* hptr{};
        CHECK_CUDA(cudaMallocHost(&hptr, count * sizeof(T)));
        return hptr;
    }

private:
    Storage m_storage{};
};

} // namespace gpu

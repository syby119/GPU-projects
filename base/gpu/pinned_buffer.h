#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

#include "gpu/utility.h"

namespace gpu {

template <typename T>
class PinnedBuffer {
public:
    PinnedBuffer() noexcept = default;

    explicit PinnedBuffer(size_t count) {
        allocate(count);
    }

    PinnedBuffer(PinnedBuffer&& other) noexcept
        : m_data{std::exchange(other.m_data, nullptr)}
        , m_size{std::exchange(other.m_size, 0)} {}

    ~PinnedBuffer() noexcept {
        LOG_CUDA(cudaFreeHost(m_data));
    }

    PinnedBuffer& operator=(PinnedBuffer&& other) noexcept {
        if (this != &other) {
            LOG_CUDA(cudaFreeHost(m_data));
            m_data = std::exchange(other.m_data, nullptr);
            m_size = std::exchange(other.m_size, 0);
        }

        return *this;
    }

    explicit operator bool() const noexcept {
        return m_data != nullptr;
    }

    T* data() noexcept {
        return m_data;
    }

    T const* data() const noexcept {
        return m_data;
    }

    size_t size() const noexcept {
        return m_size;
    }

    size_t sizeBytes() const noexcept {
        return m_size * sizeof(T);
    }

    bool empty() const noexcept {
        return m_size == 0;
    }

    void reallocate(size_t count) {
        if (count == m_size) {
            return;
        }

        PinnedBuffer temporary;
        temporary.allocate(count);
        swap(temporary);
    }

    void copyFromHost(T const* src) {
        CHECK_CUDA(cudaMemcpy(m_data, src, sizeBytes(), cudaMemcpyHostToHost));
    }

    void copyFromHost(std::span<T const> src) {
        CHECK_CUDA(cudaMemcpy(m_data, src.data(), src.size_bytes(), cudaMemcpyHostToHost));
    }

    void copyToHost(T* dst) {
        CHECK_CUDA(cudaMemcpy(dst, m_data, sizeBytes(), cudaMemcpyHostToHost));
    }

    void copyToHost(std::span<T> dst) {
        CHECK_CUDA(cudaMemcpy(dst.data(), m_data, dst.size_bytes(), cudaMemcpyHostToHost));
    }

    // Keep both buffers alive until work in the stream has completed.
    void copyFromDeviceAsync(T const* src, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(m_data, src, sizeBytes(), cudaMemcpyDeviceToHost, stream));
    }

    void copyFromDeviceAsync(std::span<T const> src, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(m_data, src.data(), src.size_bytes(), cudaMemcpyDeviceToHost, stream));
    }

    void copyToDeviceAsync(T* dst, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(dst, m_data, sizeBytes(), cudaMemcpyHostToDevice, stream));
    }

    void copyToDeviceAsync(std::span<T> dst, cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaMemcpyAsync(dst.data(), m_data, dst.size_bytes(), cudaMemcpyHostToDevice, stream));
    }

    static PinnedBuffer adopt(T* hptr, size_t count) noexcept {
        return PinnedBuffer{hptr, count};
    }

    T* release() noexcept {
        m_size = 0;
        return std::exchange(m_data, nullptr);
    }

    void swap(PinnedBuffer& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

    friend void swap(PinnedBuffer& a, PinnedBuffer& b) noexcept {
        a.swap(b);
    }

private:
    PinnedBuffer(T* hptr, size_t count) noexcept
        : m_data{hptr}
        , m_size{count} {}

    void allocate(size_t count) {
        if (count == 0) {
            return;
        }

        if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::length_error("CUDA pinned buffer size overflows size_t");
        }

        CHECK_CUDA(cudaMallocHost(&m_data, count * sizeof(T)));
        m_size = count;
    }

private:
    T* m_data{nullptr};
    size_t m_size{0};
};

} // namespace gpu

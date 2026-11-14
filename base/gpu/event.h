#pragma once

#include <utility>

#include "gpu/utility.h"

namespace gpu {

class Event {
public:
    enum class Flags : uint32_t {
        Default = cudaEventDefault,
        BlockingSync = cudaEventBlockingSync,
        DisableTiming = cudaEventDisableTiming,
    };

public:
    Event() {
        CHECK_CUDA(cudaEventCreate(&m_event));
    }

    explicit Event(Flags flags) {
        CHECK_CUDA(cudaEventCreateWithFlags(&m_event, static_cast<uint32_t>(flags)));
    }

    Event(Event&& other) noexcept
        : m_event{std::exchange(other.m_event, nullptr)} {}

    ~Event() noexcept {
        if (m_event) {
            LOG_CUDA(cudaEventDestroy(m_event));
        }
    }

    Event& operator=(Event&& other) noexcept {
        if (this != &other) {
            if (m_event) {
                LOG_CUDA(cudaEventDestroy(m_event));
            }
            m_event = std::exchange(other.m_event, nullptr);
        }

        return *this;
    }

    cudaEvent_t get() const noexcept {
        return m_event;
    }

    void record(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_event, stream));
    }

    bool isComplete() const {
        cudaError_t const status = cudaEventQuery(m_event);
        if (status == cudaErrorNotReady) {
            return false;
        }
        CHECK_CUDA(status);
        return true;
    }

    void synchronize() const {
        CHECK_CUDA(cudaEventSynchronize(m_event));
    }

private:
    cudaEvent_t m_event{nullptr};
};

} // namespace gpu

#pragma once

#include <optix/utility.h>
#include <gpu/buffer/buffer.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace optix {

template <typename Data = std::byte>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE]{};

    Data data{};
};

template <>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord<std::byte> {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE]{};
};

template <typename T>
void packHeader(OptixProgramGroup group, T& record) {
    static_assert(alignof(T) >= OPTIX_SBT_RECORD_ALIGNMENT);
    static_assert(sizeof(T) % OPTIX_SBT_RECORD_ALIGNMENT == 0);
    static_assert(std::is_standard_layout_v<T>);

    CHECK_OPTIX(optixSbtRecordPackHeader(group, &record));
}

class ShaderBindingTable {
public:
    ShaderBindingTable() = default;

    ShaderBindingTable(ShaderBindingTable&& other) noexcept;

    ShaderBindingTable& operator=(ShaderBindingTable&& other) noexcept;

    void setRaygen(void const* records, size_t stride);

    void setException(void const* records, size_t stride);

    void setMiss(void const* records, size_t stride, uint32_t count);

    void setHitgroup(void const* records, size_t stride, uint32_t count);

    void setCallables(void const* records, size_t stride, uint32_t count);

    OptixShaderBindingTable const& table() const noexcept {
        return m_table;
    }

private:
    OptixShaderBindingTable m_table{};

    gpu::Buffer<std::byte> m_raygen;
    gpu::Buffer<std::byte> m_exception;
    gpu::Buffer<std::byte> m_miss;
    gpu::Buffer<std::byte> m_hitgroup;
    gpu::Buffer<std::byte> m_callables;

private:
    static void upload(gpu::Buffer<std::byte>& target, void const* records, size_t stride, uint32_t count);
};

} // namespace optix

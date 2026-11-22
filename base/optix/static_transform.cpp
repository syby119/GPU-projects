#include <optix/static_transform.h>

#include <utility>

namespace optix {

StaticTransform::StaticTransform() noexcept
    : m_storage{decltype(m_storage)::adopt(nullptr)} {}

StaticTransform::StaticTransform(DeviceContext const& context, OptixStaticTransform const& descriptor) {
    m_storage.copyFromHost(&descriptor);
    CHECK_OPTIX(optixConvertPointerToTraversableHandle(
        context.handle(), address(m_storage.data()), OPTIX_TRAVERSABLE_TYPE_STATIC_TRANSFORM, &m_handle));
}

StaticTransform::StaticTransform(StaticTransform&& other) noexcept
    : m_storage(std::move(other.m_storage))
    , m_handle(std::exchange(other.m_handle, 0)) {}

StaticTransform& StaticTransform::operator=(StaticTransform&& other) noexcept {
    if (this != &other) {
        m_storage = std::move(other.m_storage);
        m_handle = std::exchange(other.m_handle, 0);
    }
    return *this;
}

} // namespace optix

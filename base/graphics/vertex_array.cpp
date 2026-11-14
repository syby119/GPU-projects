#include "graphics/gl_utility.h"
#include "graphics/vertex_array.h"

namespace gfx {
static GLenum toNativeDataType(VertexArray::DataType type) {
    switch (type) {
    case VertexArray::DataType::Int8:
        return GL_BYTE;
    case VertexArray::DataType::UInt8:
        return GL_UNSIGNED_BYTE;
    case VertexArray::DataType::Int16:
        return GL_SHORT;
    case VertexArray::DataType::UInt16:
        return GL_UNSIGNED_SHORT;
    case VertexArray::DataType::Int32:
        return GL_INT;
    case VertexArray::DataType::UInt32:
        return GL_UNSIGNED_INT;
    case VertexArray::DataType::Float16:
        return GL_HALF_FLOAT;
    case VertexArray::DataType::Float32:
        return GL_FLOAT;
    case VertexArray::DataType::Float64:
        return GL_DOUBLE;
    }

    throw std::runtime_error("Unsupported data type");

    return 0;
}

VertexArray::VertexArray() {
    glCreateVertexArrays(1, &m_handle);
}

VertexArray::VertexArray(VertexArray&& rhs) noexcept
    : RHIResource{std::move(rhs)} {}

VertexArray::~VertexArray() {
    if (isValid()) {
        glDeleteVertexArrays(1, &m_handle);
    }
}

VertexArray& VertexArray::operator=(VertexArray&& rhs) noexcept {
    if (this != &rhs) {
        if (isValid()) {
            glDeleteVertexArrays(1, &m_handle);
        }

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

void VertexArray::bind() const {
    glBindVertexArray(m_handle);
}

void VertexArray::unbind() {
    glBindVertexArray(0u);
}

void VertexArray::attachVertexBuffer(Buffer const& buffer, uint32_t index, intptr_t offset, size_t stride) {
    glVertexArrayVertexBuffer(m_handle, index, buffer.getNativeHandle(), offset, GLsizei(stride));
}

void VertexArray::attachIndexBuffer(Buffer const& buffer) {
    glVertexArrayElementBuffer(m_handle, buffer.getNativeHandle());
}

void VertexArray::enableAttrib(uint32_t location) {
    glEnableVertexArrayAttrib(m_handle, location);
}

void VertexArray::disableAttrib(uint32_t location) {
    glDisableVertexArrayAttrib(m_handle, location);
}

void VertexArray::setAttribFormat(
    uint32_t location,
    size_t size,
    DataType type,
    bool normalized,
    size_t relativeOffset) {
    glVertexArrayAttribFormat(
        m_handle, location, GLint(size), toNativeDataType(type), normalized, GLuint(relativeOffset));
}

void VertexArray::setAttribIFormat(uint32_t location, size_t size, DataType type, size_t relativeOffset) {
    glVertexArrayAttribIFormat(m_handle, location, GLint(size), toNativeDataType(type), GLuint(relativeOffset));
}

void VertexArray::setAttribLFormat(uint32_t location, size_t size, size_t relativeOffset) {
    glVertexArrayAttribLFormat(m_handle, location, GLint(size), GL_DOUBLE, relativeOffset);
}

void VertexArray::setAttribBinding(uint32_t location, uint32_t index) {
    glVertexArrayAttribBinding(m_handle, location, index);
}

void VertexArray::setBindingDivisor(uint32_t binding, uint32_t divisor) {
    glVertexArrayBindingDivisor(m_handle, binding, divisor);
}
} // namespace gfx

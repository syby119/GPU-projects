#include "graphics/texture/texture2d_array.h"

#include <stdexcept>

#include "graphics/gl_utility.h"
#include "graphics/texture/texture_utility.h"

namespace gfx {
Texture2DArray::Texture2DArray(Specification const& spec)
    : Texture{spec.format}
    , m_width{spec.width}
    , m_height{spec.height}
    , m_layers{spec.layers}
    , m_mipmapLevels{1}
    , m_immutable{spec.isImmutable} {
    if (m_width == 0) {
        throw std::runtime_error("Invalid texture width");
    }
    if (m_height == 0) {
        throw std::runtime_error("Invalid texture height");
    }
    if (m_layers == 0) {
        throw std::runtime_error("Invalid texture layers");
    }

    uint32_t maxMipmapLevel{getMaxMipmapLevels(m_width, m_height)};
    if (spec.generateMipmap) {
        if (spec.mipmapLevels == 0) {
            m_mipmapLevels = maxMipmapLevel;
        }
        else {
            m_mipmapLevels = std::min(maxMipmapLevel, m_mipmapLevels);
        }
    }

    GLint format{details::toNativeFormat(m_format)};
    GLenum pixelFormat{details::toNativeExternalFormat(details::chooseCompatibleExternalFormat(m_format))};
    GLenum pixelType{details::toNativeDataType(details::chooseCompatiblePixelType(m_format))};

    // reserve storage
    if (m_immutable) {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_handle);
        glTextureStorage3D(m_handle, m_mipmapLevels, format, m_width, m_height, m_layers);
    }
    else {
        glGenTextures(1, &m_handle);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, m_width, m_height, m_layers, 0, pixelFormat, pixelType, nullptr);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    // generate mipmap
    if (spec.generateMipmap) {
        if (m_immutable) {
            glGenerateTextureMipmap(m_handle);
        }
        else {
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
            glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }
    }

    checkGLErrors();
}

uint32_t Texture2DArray::getNativeType() const {
    return GL_TEXTURE_2D_ARRAY;
}

void Texture2DArray::generateMipmap() {
    if (m_mipmapLevels == 1) {
        if (m_immutable) {
            throw std::runtime_error("Generating mipmap for immutable texture needs pre-allocated memory");
        }
        m_mipmapLevels = getMaxMipmapLevels(m_width, m_height);
    }

    if (m_immutable) {
        glGenerateTextureMipmap(m_handle);
    }
    else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    checkGLErrors();
}

void Texture2DArray::update(
    void const* data,
    ExternalFormat pixelFormat,
    PixelType pixelType,
    uint32_t layer,
    uint32_t mipmaplevel) {
    if (!data) {
        return;
    }

    uint32_t width{m_width};
    uint32_t height{m_height};
    for (uint32_t i = 0; i < mipmaplevel; ++i) {
        if (width == 1 && height == 1) {
            break;
        }

        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }

    GLenum format{details::toNativeExternalFormat(pixelFormat)};
    GLenum dataType{details::toNativeDataType(pixelType)};

    // record and change data transfer alignment
    size_t const pixelSize{details::getPixelSize(pixelFormat, pixelType)};
    GLint alignment = 1;
    size_t pitch = m_width * pixelSize;
    if (pitch % 8 == 0) {
        alignment = 8;
    }
    else if (pitch % 4 == 0) {
        alignment = 4;
    }
    else if (pitch % 2 == 0) {
        alignment = 2;
    }

    GLint unpackAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

    // transfer data
    if (m_immutable) {
        glTextureSubImage3D(m_handle, mipmaplevel, 0, 0, layer, width, height, 1, format, dataType, data);
    }
    else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipmaplevel, 0, 0, layer, width, height, 1, format, dataType, data);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    // restore alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    checkGLErrors();
}

uint32_t Texture2DArray::getMaxMipmapLevels(uint32_t width, uint32_t height) {
    uint32_t maxMipmapLevel{0};
    for (uint32_t extent = std::max(width, height); extent != 1; extent /= 2) {
        ++maxMipmapLevel;
    }

    return 1 + maxMipmapLevel;
}
} // namespace gfx

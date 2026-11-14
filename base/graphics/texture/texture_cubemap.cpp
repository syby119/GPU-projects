#include "graphics/texture/texture_cubemap.h"

#include <stdexcept>

#include "graphics/gl_utility.h"
#include "graphics/texture/texture_utility.h"

namespace gfx {
TextureCubemap::TextureCubemap(Specification const& spec, std::array<void const*, 6> const& buffers)
    : Texture{spec.format}
    , m_width{spec.width}
    , m_height{spec.height}
    , m_mipmapLevels{1}
    , m_immutable{spec.isImmutable} {
    if (m_width == 0) {
        throw std::runtime_error("Invalid texture width");
    }
    if (m_height == 0) {
        throw std::runtime_error("Invalid texture height");
    }

    if (m_format == Texture::Format::None) {
        m_format = details::chooseCompatibleFormat(spec.pixelFormat, spec.pixelType);
    }

    auto const maxMipmapLevel{details::getMaxMipmapLevels(m_width, m_height)};
    if (spec.generateMipmap) {
        if (spec.mipmapLevels == 0) {
            m_mipmapLevels = maxMipmapLevel;
        }
        else {
            m_mipmapLevels = std::min(maxMipmapLevel, spec.mipmapLevels);
        }
    }

    GLint const format{details::toNativeFormat(m_format)};
    GLenum const pixelFormat{details::toNativeExternalFormat(spec.pixelFormat)};
    GLenum const dataType{details::toNativeDataType(spec.pixelType)};

    // pre allocate storage
    if (m_immutable) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_handle);
        glTextureStorage2D(m_handle, m_mipmapLevels, format, m_width, m_height);
    }
    else {
        glGenTextures(1, &m_handle);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
        for (uint32_t i = 0; i < 6; ++i) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, m_width, m_height, 0, pixelFormat, dataType, nullptr);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    checkGLErrors();

    // transfer data
    for (uint32_t i = 0; i < 6; ++i) {
        update(buffers[i], spec.pixelFormat, spec.pixelType, static_cast<Face>(i), 0);
        checkGLErrors();
    }

    // generate mipmaps
    if (spec.generateMipmap) {
        if (m_immutable) {
            glGenerateTextureMipmap(m_handle);
        }
        else {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
    }

    checkGLErrors();
}

uint32_t TextureCubemap::getNativeType() const {
    return GL_TEXTURE_CUBE_MAP;
}

void TextureCubemap::update(
    void const* data,
    Texture::ExternalFormat pixelFormat,
    Texture::PixelType pixelType,
    Face face,
    uint32_t level) {
    if (data == nullptr) {
        return;
    }

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

    // data transfer
    uint32_t width{m_width};
    uint32_t height{m_height};
    for (uint32_t i = 0; i < level; ++i) {
        if (width == 1 && height == 1) {
            break;
        }

        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }

    GLenum format{details::toNativeExternalFormat(pixelFormat)};
    GLenum dataType{details::toNativeDataType(pixelType)};

    if (m_immutable) {
        glTextureSubImage3D(m_handle, level, 0, 0, static_cast<int>(face), width, height, 1, format, dataType, data);
    }
    else {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
        glTexSubImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<uint32_t>(face),
            level,
            0,
            0,
            width,
            height,
            format,
            dataType,
            data);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    // restore the alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    checkGLErrors();
}

void TextureCubemap::generateMipmap() {
    if (m_mipmapLevels == 1) {
        if (m_immutable) {
            throw std::runtime_error("Generating mipmap for immutable texture needs pre-allocated memory");
        }
        m_mipmapLevels = details::getMaxMipmapLevels(m_width, m_height);
    }

    if (m_immutable) {
        glGenerateTextureMipmap(m_handle);
    }
    else {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    checkGLErrors();
}
} // namespace gfx

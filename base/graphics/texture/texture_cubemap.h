#pragma once

#include <array>

#include "graphics/texture/texture.h"

namespace gfx {
class TextureCubemap : public Texture {
public:
    enum class Face {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,
    };

public:
    struct Specification {
        uint32_t width{0};
        uint32_t height{0};
        Texture::Format format{Texture::Format::None};
        Texture::ExternalFormat pixelFormat{Texture::ExternalFormat::None};
        Texture::PixelType pixelType{Texture::PixelType::None};
        bool generateMipmap{false};
        uint32_t mipmapLevels{0};
        bool isImmutable{false};
    };

public:
    TextureCubemap(Specification const& spec, std::array<void const*, 6> const& buffers = {nullptr});

    TextureCubemap(TextureCubemap&& rhs) noexcept = default;

    ~TextureCubemap() override = default;

    TextureCubemap& operator=(TextureCubemap&& rhs) noexcept = default;

    uint32_t getNativeType() const override;

    void update(
        void const* data,
        Texture::ExternalFormat pixelFormat,
        Texture::PixelType pixelType,
        Face face,
        uint32_t level = 0);

    void generateMipmap();

private:
    uint32_t m_width{0};
    uint32_t m_height{0};
    uint32_t m_mipmapLevels{1};
    bool m_immutable{false};
};
} // namespace gfx

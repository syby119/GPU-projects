#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "graphics/rhi_resource.h"
#include "graphics/renderbuffer.h"
#include "graphics/texture/texture.h"

namespace gfx {
class Framebuffer : public RHIResource {
public:
    Framebuffer();

    Framebuffer(Framebuffer&& rhs) noexcept = default;

    ~Framebuffer();

    Framebuffer& operator=(Framebuffer&& rhs) noexcept;

    void bind();

    void bindRead();

    void bindWrite();

    static void unbind();

    static void unbindRead();

    static void unbindWrite();

    void attachAsColor(uint32_t index, Texture const& texture, int level = 0);

    void attachAsDepth(Texture const& texture, int level = 0);

    void attachAsStencil(Texture const& texture, int level = 0);

    void attachAsDepthStencil(Texture const& texture, int level = 0);

    void attachAsColor(uint32_t index, Renderbuffer const& rbo);

    void attachAsDepth(Renderbuffer const& rbo);

    void attachAsStencil(Renderbuffer const& rbo);

    void attachAsDepthStencil(Renderbuffer const& rbo);

    void attachLayerAsColor(uint32_t index, Texture const& texture, int layer, int level = 0);

    void attachLayerAsDepth(Texture const& texture, int layer, int level = 0);

    void attachLayerAsStencil(Texture const& texture, int layer, int level = 0);

    void attachLayerAsDepthStencil(Texture const& texture, int layer, int level = 0);

    bool isComplete() const;

    std::string getDiagnostic() const;

    void setColorWrite(uint32_t index) const;

    void setColorWrites(std::vector<uint32_t> const& indices);

    void disableColorWrite();

    void setColorRead(uint32_t index);

    void disableColorRead();

    void clearColorBuffer(uint32_t index, glm::ivec4 const& color);

    void clearColorBuffer(uint32_t index, glm::uvec4 const& color);

    void clearColorBuffer(uint32_t index, glm::vec4 const& color);

    void clearDepthBuffer(float depth);

    void clearStencilBuffer(int stencil);

    void clearDepthStencilBuffer(float depth, int stencil);

    // non DSA API
    static void readBackBuffer();

    static void readPixels(
        int x,
        int y,
        int width,
        int height,
        Texture::ExternalFormat format,
        Texture::PixelType type,
        void* data);

    // TODO: Support default framebuffer
    // static Framebuffer getDefaultFramebuffer();
};
} // namespace gfx

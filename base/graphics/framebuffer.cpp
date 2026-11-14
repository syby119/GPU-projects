#include "graphics/framebuffer.h"
#include "graphics/gl_utility.h"
#include "graphics/texture/texture_utility.h"

namespace gfx {
Framebuffer::Framebuffer() {
    glCreateFramebuffers(1, &m_handle);
}

Framebuffer::~Framebuffer() {
    if (m_handle != 0) {
        glDeleteFramebuffers(1, &m_handle);
    }
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs) noexcept {
    if (this != &rhs) {
        if (isValid()) {
            glDeleteFramebuffers(1, &m_handle);
        }

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void Framebuffer::bindRead() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_handle);
}

void Framebuffer::bindWrite() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_handle);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(0));
}

void Framebuffer::unbindRead() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(0));
}

void Framebuffer::unbindWrite() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(0));
}

void Framebuffer::attachAsColor(uint32_t index, Texture const& texture, int level) {
    GLenum const attachment{GL_COLOR_ATTACHMENT0 + index};
    glNamedFramebufferTexture(m_handle, attachment, texture.getNativeHandle(), level);
}

void Framebuffer::attachAsDepth(Texture const& texture, int level) {
    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, texture.getNativeHandle(), level);
}

void Framebuffer::attachAsStencil(Texture const& texture, int level) {
    glNamedFramebufferTexture(m_handle, GL_STENCIL_ATTACHMENT, texture.getNativeHandle(), level);
}

void Framebuffer::attachAsDepthStencil(Texture const& texture, int level) {
    glNamedFramebufferTexture(m_handle, GL_DEPTH_STENCIL_ATTACHMENT, texture.getNativeHandle(), level);
}

void Framebuffer::attachAsColor(uint32_t index, Renderbuffer const& rbo) {
    GLenum const attachment{GL_COLOR_ATTACHMENT0 + index};
    glNamedFramebufferRenderbuffer(m_handle, attachment, GL_RENDERBUFFER, rbo.getNativeHandle());
}

void Framebuffer::attachAsDepth(Renderbuffer const& rbo) {
    glNamedFramebufferRenderbuffer(m_handle, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo.getNativeHandle());
}

void Framebuffer::attachAsStencil(Renderbuffer const& rbo) {
    glNamedFramebufferRenderbuffer(m_handle, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo.getNativeHandle());
}

void Framebuffer::attachAsDepthStencil(Renderbuffer const& rbo) {
    glNamedFramebufferRenderbuffer(m_handle, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo.getNativeHandle());
}

void Framebuffer::attachLayerAsColor(uint32_t index, Texture const& texture, int layer, int level) {
    GLenum const attachment{GL_COLOR_ATTACHMENT0 + index};
    glNamedFramebufferTextureLayer(m_handle, attachment, texture.getNativeHandle(), level, layer);
}

void Framebuffer::attachLayerAsDepth(Texture const& texture, int layer, int level) {
    glNamedFramebufferTextureLayer(m_handle, GL_DEPTH_ATTACHMENT, texture.getNativeHandle(), level, layer);
}

void Framebuffer::attachLayerAsStencil(Texture const& texture, int layer, int level) {
    glNamedFramebufferTextureLayer(m_handle, GL_STENCIL_ATTACHMENT, texture.getNativeHandle(), level, layer);
}

void Framebuffer::attachLayerAsDepthStencil(Texture const& texture, int layer, int level) {
    glNamedFramebufferTextureLayer(m_handle, GL_DEPTH_STENCIL_ATTACHMENT, texture.getNativeHandle(), level, layer);
}

bool Framebuffer::isComplete() const {
    return glCheckNamedFramebufferStatus(m_handle, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

std::string Framebuffer::getDiagnostic() const {
    GLenum status{glCheckNamedFramebufferStatus(m_handle, GL_FRAMEBUFFER)};
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return "framebuffer: complete";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "framebuffer: incomplete attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "framebuffer: missing attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "framebuffer: incomplete draw buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "framebuffer: incomplete layer targets";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "framebuffer: incomplete multisample";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "framebuffer: incomplete read buffer";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "framebuffer: unsupported";
    case GL_FRAMEBUFFER_UNDEFINED:
        return "framebuffer: undefined";
    }

    return "framebuffer: unknown";
}

void Framebuffer::setColorWrite(uint32_t index) const {
    GLenum colorAttachment{GL_COLOR_ATTACHMENT0 + index};
    glNamedFramebufferDrawBuffers(m_handle, 1, &colorAttachment);
}

void Framebuffer::setColorWrites(std::vector<uint32_t> const& indices) {
    std::vector<GLenum> colorAttachments;
    colorAttachments.reserve(indices.size());
    for (auto index : indices) {
        colorAttachments.push_back(GL_COLOR_ATTACHMENT0 + index);
    }

    glNamedFramebufferDrawBuffers(m_handle, (GLsizei)colorAttachments.size(), colorAttachments.data());
}

void Framebuffer::disableColorWrite() {
    GLenum buffer{GL_NONE};
    glNamedFramebufferDrawBuffers(m_handle, 1, &buffer);
}

void Framebuffer::setColorRead(uint32_t index) {
    glNamedFramebufferReadBuffer(m_handle, GL_COLOR_ATTACHMENT0 + index);
}

void Framebuffer::disableColorRead() {
    glNamedFramebufferReadBuffer(m_handle, GL_NONE);
}

void Framebuffer::clearColorBuffer(uint32_t index, glm::ivec4 const& color) {
    glClearNamedFramebufferiv(m_handle, GL_COLOR, index, &color[0]);
}

void Framebuffer::clearColorBuffer(uint32_t index, glm::uvec4 const& color) {
    glClearNamedFramebufferuiv(m_handle, GL_COLOR, index, &color[0]);
}

void Framebuffer::clearColorBuffer(uint32_t index, glm::vec4 const& color) {
    glClearNamedFramebufferfv(m_handle, GL_COLOR, index, &color[0]);
}

void Framebuffer::clearDepthBuffer(float depth) {
    glClearNamedFramebufferfv(m_handle, GL_DEPTH, 0, &depth);
}

void Framebuffer::clearStencilBuffer(int stencil) {
    glClearNamedFramebufferiv(m_handle, GL_STENCIL, 0, &stencil);
}

void Framebuffer::clearDepthStencilBuffer(float depth, int stencil) {
    glClearNamedFramebufferfi(m_handle, GL_DEPTH_STENCIL, 0, depth, stencil);
}

void Framebuffer::readBackBuffer() {
    glReadBuffer(GL_BACK);
}

void Framebuffer::readPixels(
    int x,
    int y,
    int width,
    int height,
    Texture::ExternalFormat format,
    Texture::PixelType type,
    void* data) {
    // Note we need to set read buffer before read pixel: glReadBuffer(xxx);
    // Also, don't forget to bind the framebuffer
    GLenum glFormat{details::toNativeExternalFormat(format)};
    GLenum glType{details::toNativeDataType(type)};
    glReadPixels(x, y, width, height, glFormat, glType, data);
}
} // namespace gfx

#include "graphics/rhi.h"
#include "graphics/gl_utility.h"

namespace gfx {
static GLenum toNativePrimitive(RHI::Primitive primitive) {
    switch (primitive) {
    case RHI::Primitive::Points:
        return GL_POINTS;
    case RHI::Primitive::Lines:
        return GL_LINES;
    case RHI::Primitive::LineStrip:
        return GL_LINE_STRIP;
    case RHI::Primitive::LineLoop:
        return GL_LINE_LOOP;
    case RHI::Primitive::Triangles:
        return GL_TRIANGLES;
    case RHI::Primitive::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case RHI::Primitive::TriangleFan:
        return GL_TRIANGLE_FAN;
    }

    throw std::logic_error("Unsupported primitive type");

    return 0;
}

static GLenum toNativeBlendFactor(RHI::BlendFactor factor) {
    /* @ref https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml
     * + first source  (Rs0, Gs0, Bs0, As0)
     * + second source (Rs1, Gs1, Bs1, As1)
     * + destination   (Rd, Gd, Bd, Ad)
     * + glBlendColor  (Rc, Gc, Bc, Ac)
     * ---------------------------------------------------------------------------------
     *  Mode                          Scalar Factor [0, 1]
     *  --------------------------------------------------------------------------------
     *  GL_ZERO                       (0, 0, 0, 0)
     *  GL_ONE                        (1, 1, 1, 1)
     *  GL_SRC_COLOR                  (Rs0, Gs0, Bs0, As0)
     *  GL_ONE_MINUS_SRC_COLOR        (1, 1, 1, 1) - (Rs0, Rs0, Bs0, As0)
     *  GL_DST_COLOR,                 (Rd, Gd, Bd, Ad)
     *  GL_ONE_MINUS_DST_COLOR        (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
     *  GL_SRC_ALPHA                  (As0, As0, As0, As0)
     *  GL_ONE_MINUS_SRC_ALPHA        (1, 1, 1, 1) - (As0, As0, As0, As0)
     *  GL_DST_ALPHA                  (Ad, Ad, Ad, Ad)
     *  GL_ONE_MINUS_DST_ALPHA        (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
     *  GL_CONSTANT_COLOR             (Rc, Gc, Bc, Ac)
     *  GL_ONE_MINUS_CONSTANT_COLOR   (1, 1, 1, 1) - (Rc, Gc, Bc, Ac)
     *  GL_CONSTANT_ALPHA             (Ac, Ac, Ac, Ac)
     *  GL_ONE_MINUS_CONSTANT_ALPHA   (1, 1, 1, 1) - (Ac, Ac, Ac, Ac)
     *  GL_SRC_ALPHA_SATURATE         (min(ks, kA-Ad), min(ks, kA-Ad), min(ks, kA-Ad), 1)
     *  GL_SRC1_COLOR                 (Rs1, Gs1, Bs1, As1)
     *  GL_ONE_MINUS_SRC1_COLOR       (1, 1, 1, 1) - (Rs1, Gs1, Bs1, As1)
     *  GL_SRC1_ALPHA                 (As1, As1, As1, As1)
     *  GL_ONE_MINUS_SRC1_ALPHA       (1, 1, 1, 1) - (As1, As1, As1, As1)
     */
    switch (factor) {
    case RHI::BlendFactor::Zero:
        return GL_ZERO;
    case RHI::BlendFactor::One:
        return GL_ONE;
    case RHI::BlendFactor::SrcColor:
        return GL_SRC_COLOR;
    case RHI::BlendFactor::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case RHI::BlendFactor::DstColor:
        return GL_DST_COLOR;
    case RHI::BlendFactor::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case RHI::BlendFactor::SrcAlpha:
        return GL_SRC_ALPHA;
    case RHI::BlendFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case RHI::BlendFactor::DstAlpha:
        return GL_DST_ALPHA;
    case RHI::BlendFactor::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case RHI::BlendFactor::ConstantColor:
        return GL_CONSTANT_COLOR;
    case RHI::BlendFactor::OneMinusConstantColor:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case RHI::BlendFactor::ConstantAlpha:
        return GL_CONSTANT_ALPHA;
    case RHI::BlendFactor::OneMinusConstantAlpha:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    case RHI::BlendFactor::SrcAlphaSaturate:
        return GL_SRC_ALPHA_SATURATE;
    case RHI::BlendFactor::Src1Color:
        return GL_SRC1_COLOR;
    case RHI::BlendFactor::OneMinusSrc1Color:
        return GL_ONE_MINUS_SRC1_COLOR;
    case RHI::BlendFactor::Src1Alpha:
        return GL_SRC1_ALPHA;
    case RHI::BlendFactor::OneMinusSrc1Alpha:
        return GL_ONE_MINUS_SRC1_ALPHA;
    }

    throw std::logic_error("Unsupported blend factor");

    return 0;
};

static GLenum toNativeFaceMode(RHI::FaceMode mode) {
    switch (mode) {
    case RHI::FaceMode::Front:
        return GL_FRONT;
    case RHI::FaceMode::Back:
        return GL_BACK;
    case RHI::FaceMode::FrontAndBack:
        return GL_FRONT_AND_BACK;
    }

    throw std::logic_error("Unsupported face mode");

    return 0;
}

static GLenum toNativePolygonMode(RHI::PolygonMode mode) {
    switch (mode) {
    case RHI::PolygonMode::Fill:
        return GL_FILL;
    case RHI::PolygonMode::Line:
        return GL_LINE;
    case RHI::PolygonMode::Point:
        return GL_POINT;
    }

    throw std::logic_error("Unsupported polygon mode");

    return 0;
}

static GLenum toNativeCompareFunc(RHI::CompareFunc func) {
    switch (func) {
    case RHI::CompareFunc::Less:
        return GL_LESS;
    case RHI::CompareFunc::LessEqual:
        return GL_LEQUAL;
    case RHI::CompareFunc::Greater:
        return GL_GREATER;
    case RHI::CompareFunc::GreaterEqual:
        return GL_GEQUAL;
    case RHI::CompareFunc::Equal:
        return GL_EQUAL;
    case RHI::CompareFunc::NotEqual:
        return GL_NOTEQUAL;
    case RHI::CompareFunc::Always:
        return GL_ALWAYS;
    case RHI::CompareFunc::Never:
        return GL_NEVER;
    }

    throw std::logic_error("Unsupported compare function");

    return 0;
}

static GLenum toNativeBlendEquation(RHI::BlendEquation equation) {
    switch (equation) {
    case RHI::BlendEquation::Add:
        return GL_FUNC_ADD;
    case RHI::BlendEquation::Subtract:
        return GL_FUNC_SUBTRACT;
    case RHI::BlendEquation::ReverseSubtract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case RHI::BlendEquation::Min:
        return GL_MIN;
    case RHI::BlendEquation::Max:
        return GL_MAX;
    }

    throw std::logic_error("Unsupported blend equation");

    return 0;
}

static GLenum toNativeStencilOp(RHI::StencilOp op) {
    switch (op) {
    case RHI::StencilOp::Keep:
        return GL_KEEP;
    case RHI::StencilOp::Zero:
        return GL_ZERO;
    case RHI::StencilOp::Replace:
        return GL_REPLACE;
    case RHI::StencilOp::Increment:
        return GL_INCR;
    case RHI::StencilOp::IncrementWrap:
        return GL_INCR_WRAP;
    case RHI::StencilOp::Decrement:
        return GL_DECR;
    case RHI::StencilOp::DecrementWrap:
        return GL_DECR_WRAP;
    case RHI::StencilOp::Invert:
        return GL_INVERT;
    }

    throw std::logic_error("Unsupported stencil op");

    return 0;
}

Viewport RHI::getViewport() {
    GLint glViewportRect[4]{0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, glViewportRect);

    return Viewport(
        static_cast<float>(glViewportRect[0]),
        static_cast<float>(glViewportRect[1]),
        static_cast<float>(glViewportRect[2]),
        static_cast<float>(glViewportRect[3]),
        0.0f,
        1.0f);
}

void RHI::setViewport(Viewport const& viewport) {
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
}

void RHI::clear(bool color, bool depth, bool stencil) {
    GLbitfield mask{0};
    if (color) {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (depth) {
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (stencil) {
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(mask);
}

glm::vec4 RHI::getClearColor() {
    glm::vec4 color;
    glGetFloatv(GL_COLOR_CLEAR_VALUE, &color[0]);

    return color;
}

void RHI::setClearColor(glm::vec4 const& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void RHI::setClearDepth(float depth) {
    glClearDepth(depth);
}

void RHI::setClearStencil(int stencil) {
    glClearStencil(stencil);
}

void RHI::enableColorWrite(bool r, bool g, bool b, bool a) {
    glColorMask(
        static_cast<GLboolean>(r), static_cast<GLboolean>(g), static_cast<GLboolean>(b), static_cast<GLboolean>(a));
}

void RHI::enableDepthTest(bool enable) {
    enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

void RHI::enableDepthWrite(bool enable) {
    glDepthMask(static_cast<GLboolean>(enable));
}

void RHI::setDepthFunc(CompareFunc func) {
    glDepthFunc(toNativeCompareFunc(func));
}

void RHI::enableStencilTest(bool enable) {
    enable ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
}

void RHI::setStencilFunc(CompareFunc func, int ref, uint32_t mask) {
    glStencilFunc(toNativeCompareFunc(func), ref, mask);
}

void RHI::setStencilMask(uint32_t mask) {
    glStencilMask(mask);
}

void RHI::setStencilMaskSeperate(FaceMode mode, uint32_t mask) {
    glStencilMaskSeparate(toNativeFaceMode(mode), mask);
}

void RHI::setStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
    glStencilOp(toNativeStencilOp(sfail), toNativeStencilOp(dpfail), toNativeStencilOp(dppass));
}

void RHI::setStencilOpSeperate(FaceMode mode, StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
    glStencilOpSeparate(
        toNativeFaceMode(mode), toNativeStencilOp(sfail), toNativeStencilOp(dpfail), toNativeStencilOp(dppass));
}

void RHI::enableScissorTest(bool enable) {
    enable ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
}

void RHI::setPolygonMode(FaceMode mode, PolygonMode polygonMode) {
    glPolygonMode(toNativeFaceMode(mode), toNativePolygonMode(polygonMode));
}

void RHI::enableCullFace(bool enable) {
    enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
}

void RHI::enableRasterizerDiscard(bool enable) {
    enable ? glEnable(GL_RASTERIZER_DISCARD) : glDisable(GL_RASTERIZER_DISCARD);
}

void RHI::setCullFace(FaceMode mode) {
    glCullFace(toNativeFaceMode(mode));
}

void RHI::setFrontFaceOrientation(bool clockwise) {
    clockwise ? glFrontFace(GL_CW) : glFrontFace(GL_CCW);
}

void RHI::enableBlend(bool enable) {
    enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

void RHI::setBlendEquation(BlendEquation equation) {
    glBlendEquation(toNativeBlendEquation(equation));
}

void RHI::setBlendEquation(uint32_t index, BlendEquation equation) {
    glBlendEquationi(index, toNativeBlendEquation(equation));
}

void RHI::setBlendColor(glm::vec4 const& color) {
    glBlendColor(color.r, color.g, color.b, color.a);
}

void RHI::setBlendFunc(BlendFactor srcFactor, BlendFactor dstFactor) {
    // Pixels can be drawn using a function that blends the incoming(source) RGBA values with
    // the RGBA values that are already in the frame buffer(the destination values).
    GLenum const sfactor{toNativeBlendFactor(srcFactor)};
    GLenum const dfactor{toNativeBlendFactor(dstFactor)};

    glBlendFunc(sfactor, dfactor);
}

void RHI::setBlendFunc(uint32_t index, BlendFactor srcFactor, BlendFactor dstFactor) {
    GLenum const sfactor{toNativeBlendFactor(srcFactor)};
    GLenum const dfactor{toNativeBlendFactor(dstFactor)};

    glBlendFunci(index, sfactor, dfactor);
}

void RHI::setBlendFuncSeperate(BlendFactor rgbSrc, BlendFactor rgbDst, BlendFactor alphaSrc, BlendFactor alphaDst) {
    GLenum const sfactorRGB{toNativeBlendFactor(rgbSrc)};
    GLenum const dfactorRGB{toNativeBlendFactor(rgbDst)};
    GLenum const sfactorAlpha{toNativeBlendFactor(alphaSrc)};
    GLenum const dfactorAlpha{toNativeBlendFactor(alphaDst)};

    glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

void RHI::setBlendFuncSeperate(
    uint32_t index,
    BlendFactor rgbSrc,
    BlendFactor rgbDst,
    BlendFactor alphaSrc,
    BlendFactor alphaDst) {
    GLenum const sfactorRGB{toNativeBlendFactor(rgbSrc)};
    GLenum const dfactorRGB{toNativeBlendFactor(rgbDst)};
    GLenum const sfactorAlpha{toNativeBlendFactor(alphaSrc)};
    GLenum const dfactorAlpha{toNativeBlendFactor(alphaDst)};

    glBlendFuncSeparatei(index, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

void RHI::enableCubemapSeamless(bool enable) {
    enable ? glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS) : glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void RHI::setPointSize(float size) {
    glPointSize(size);
}

void RHI::setLineWidth(float width) {
    glLineWidth(width);
}

void RHI::enablePolygonOffsetFill(bool enable) {
    enable ? glEnable(GL_POLYGON_OFFSET_FILL) : glDisable(GL_POLYGON_OFFSET_FILL);
}

void RHI::enablePolygonOffsetLine(bool enable) {
    enable ? glEnable(GL_POLYGON_OFFSET_LINE) : glDisable(GL_POLYGON_OFFSET_LINE);
}

void RHI::enablePolygonOffsetPoint(bool enable) {
    enable ? glEnable(GL_POLYGON_OFFSET_POINT) : glDisable(GL_POLYGON_OFFSET_POINT);
}

void RHI::setPolygonOffset(float factor, float units) {
    glPolygonOffset(factor, units);
}

void RHI::draw(Primitive pt, int first, uint32_t count) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawArrays(mode, first, (GLsizei)count);
}

void RHI::drawInstanced(Primitive pt, int first, uint32_t count, uint32_t instCnt) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawArraysInstanced(mode, first, (GLsizei)count, (GLsizei)instCnt);
}

void RHI::drawIndexed(Primitive pt, int first, uint32_t count) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawElements(mode, (GLsizei)count, GL_UNSIGNED_INT, (GLvoid*)(first * sizeof(uint32_t)));
}

void RHI::drawIndexedInstanced(Primitive pt, int first, uint32_t count, uint32_t instCnt) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawElementsInstanced(
        mode, (GLsizei)count, GL_UNSIGNED_INT, (GLvoid*)(first * sizeof(uint32_t)), (GLsizei)instCnt);
}

void RHI::drawMeshTasksNV(uint32_t first, uint32_t count) {
    glDrawMeshTasksNV(first, count);
}

void RHI::dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

void RHI::finish() {
    glFinish();
}

void RHI::drawIndexed(Primitive pt, uint32_t count, uint32_t const* indices) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawElements(mode, count, GL_UNSIGNED_INT, indices);
}

void RHI::drawIndexedInstanced(Primitive pt, uint32_t count, uint32_t const* indices, uint32_t instCnt) {
    auto const mode{static_cast<GLenum>(toNativePrimitive(pt))};
    glDrawElementsInstanced(mode, (GLsizei)count, GL_UNSIGNED_INT, indices, (GLsizei)instCnt);
}

void RHI::memoryBarrier(MemoryBarrierBits barriers) {
    if (barriers == MemoryBarrierBits::All) {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        return;
    }

    GLbitfield mask{0};
    if (testBitmaskContain(barriers, MemoryBarrierBits::VertexAttribArray)) {
        mask |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::ElementArray)) {
        mask |= GL_ELEMENT_ARRAY_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::Uniform)) {
        mask |= GL_UNIFORM_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::TextureFetch)) {
        mask |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::ShaderImageAccess)) {
        mask |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::Command)) {
        mask |= GL_COMMAND_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::PixelBuffer)) {
        mask |= GL_PIXEL_BUFFER_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::TextureUpdate)) {
        mask |= GL_TEXTURE_UPDATE_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::BufferUpdate)) {
        mask |= GL_BUFFER_UPDATE_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::ClientMappedBuffer)) {
        mask |= GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::Framebuffer)) {
        mask |= GL_FRAMEBUFFER_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::TransformFeedback)) {
        mask |= GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::AtomicCounter)) {
        mask |= GL_ATOMIC_COUNTER_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::ShaderStorage)) {
        mask |= GL_SHADER_STORAGE_BARRIER_BIT;
    }
    if (testBitmaskContain(barriers, MemoryBarrierBits::QueryBuffer)) {
        mask |= GL_QUERY_BUFFER_BARRIER_BIT;
    }

    glMemoryBarrier(mask);
}

void RHI::insertDebugMessage(std::string const& message) {
#ifndef NDEBUG
    glDebugMessageInsert(
        GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, message.c_str());
#endif
}
} // namespace gfx

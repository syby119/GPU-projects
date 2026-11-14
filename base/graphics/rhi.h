#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include <glm/glm.hpp>

#include "common/enum.h"
#include "graphics/viewport.h"

namespace gfx {
class RHI {
public:
    enum class Primitive {
        Points,
        Lines,
        LineStrip,
        LineLoop,
        Triangles,
        TriangleStrip,
        TriangleFan,
    };

    enum class FaceMode { Front, Back, FrontAndBack };

    enum class PolygonMode { Fill, Line, Point };

    enum class CompareFunc { Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual, Always, Never };

    enum class BlendFactor {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
        Src1Color,
        OneMinusSrc1Color,
        Src1Alpha,
        OneMinusSrc1Alpha
    };

    enum class BlendEquation { Add, Subtract, ReverseSubtract, Min, Max };

    enum class StencilOp { Keep, Zero, Replace, Increment, IncrementWrap, Decrement, DecrementWrap, Invert };

    enum class MemoryBarrierBits : uint32_t {
        VertexAttribArray = makeBitmaskBit(0u),
        ElementArray = makeBitmaskBit(1u),
        Uniform = makeBitmaskBit(2u),
        TextureFetch = makeBitmaskBit(3u),
        ShaderImageAccess = makeBitmaskBit(4u),
        Command = makeBitmaskBit(5u),
        PixelBuffer = makeBitmaskBit(6u),
        TextureUpdate = makeBitmaskBit(7u),
        BufferUpdate = makeBitmaskBit(8u),
        ClientMappedBuffer = makeBitmaskBit(9u),
        Framebuffer = makeBitmaskBit(10u),
        TransformFeedback = makeBitmaskBit(11u),
        AtomicCounter = makeBitmaskBit(12u),
        ShaderStorage = makeBitmaskBit(13u),
        QueryBuffer = makeBitmaskBit(14u),
        All = std::numeric_limits<uint32_t>::max()
    };

public:
    // viewport
    static Viewport getViewport();

    static void setViewport(Viewport const& viewport);

    // clear
    static void clear(bool color, bool depth, bool stencil = false);

    static glm::vec4 getClearColor();

    static void setClearColor(glm::vec4 const& color);

    static void setClearDepth(float depth);

    static void setClearStencil(int stencil);

    // color
    static void enableColorWrite(bool r, bool g, bool b, bool a);

    // depth
    static void enableDepthTest(bool enable);

    static void enableDepthWrite(bool enable);

    static void setDepthFunc(CompareFunc func);

    // stencil
    static void enableStencilTest(bool enable);

    static void setStencilFunc(CompareFunc func, int ref, uint32_t mask);

    static void setStencilMask(uint32_t mask);

    static void setStencilMaskSeperate(FaceMode mode, uint32_t mask);

    static void setStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass);

    static void setStencilOpSeperate(FaceMode mode, StencilOp sfail, StencilOp dpfail, StencilOp dppass);

    // scissor
    static void enableScissorTest(bool enable);

    // polygon mode
    static void setPolygonMode(FaceMode mode, PolygonMode polygonMode);

    // cull face
    static void enableCullFace(bool enable);

    static void setCullFace(FaceMode mode);

    static void setFrontFaceOrientation(bool clockwise);

    // rasterization
    static void enableRasterizerDiscard(bool enable);

    // blend
    static void enableBlend(bool enable);

    static void setBlendEquation(BlendEquation equation);

    static void setBlendEquation(uint32_t index, BlendEquation equation);

    static void setBlendColor(glm::vec4 const& color);

    static void setBlendFunc(BlendFactor srcFactor, BlendFactor dstFactor);

    static void setBlendFunc(uint32_t index, BlendFactor srcFactor, BlendFactor dstFactor);

    static void
    setBlendFuncSeperate(BlendFactor rgbSrc, BlendFactor rgbDst, BlendFactor alphaSrc, BlendFactor alphaDst);

    static void setBlendFuncSeperate(
        uint32_t index,
        BlendFactor rgbSrc,
        BlendFactor rgbDst,
        BlendFactor alphaSrc,
        BlendFactor alphaDst);

    // cubemap
    static void enableCubemapSeamless(bool enable);

    // primitive size
    static void setPointSize(float size);

    static void setLineWidth(float width);

    // polygon offset
    static void enablePolygonOffsetFill(bool enable);

    static void enablePolygonOffsetLine(bool enable);

    static void enablePolygonOffsetPoint(bool enable);

    static void setPolygonOffset(float factor, float units);

    // draw calls
    static void draw(Primitive pt, int first, uint32_t count);

    static void drawInstanced(Primitive pt, int first, uint32_t count, uint32_t instCnt);

    static void drawIndexed(Primitive pt, int first, uint32_t count);

    static void drawIndexedInstanced(Primitive pt, int first, uint32_t count, uint32_t instCnt);

    static void drawMeshTasksNV(uint32_t first, uint32_t count);

    static void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);

    static void finish();

    // OpenGL legacy API
    static void drawIndexed(Primitive pt, uint32_t count, uint32_t const* indices);

    static void drawIndexedInstanced(Primitive pt, uint32_t count, uint32_t const* indices, uint32_t instCnt);

    // memory barrier
    static void memoryBarrier(MemoryBarrierBits barriers);

    // debug
    static void insertDebugMessage(std::string const& message);
};

ENABLE_BITMASK_OPERATION(RHI::MemoryBarrierBits);
} // namespace gfx

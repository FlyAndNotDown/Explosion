//
// Created by johnk on 2026/7/8.
//

#include <algorithm>

#include <Runtime/Canvas.h>

namespace Runtime::Internal {
    static uint32_t ClampExtent(uint32_t inValue)
    {
        return std::max(1u, inValue);
    }
}

namespace Runtime {
    CanvasDesc::CanvasDesc()
        : width(1)
        , height(1)
        , format(RHI::PixelFormat::rgba8Unorm)
        , debugName("canvas")
    {
    }

    Canvas::Canvas(RHI::Device& inDevice, const CanvasDesc& inDesc)
        : device(inDevice)
        , width(Internal::ClampExtent(inDesc.width))
        , height(Internal::ClampExtent(inDesc.height))
        , format(inDesc.format)
        , debugName(inDesc.debugName)
    {
        CreateTarget();
    }

    Canvas::~Canvas() = default;

    RHI::Texture* Canvas::GetTexture() const
    {
        return texture.Get();
    }

    RHI::TextureView* Canvas::GetRenderTargetView() const
    {
        return renderTargetView.Get();
    }

    RHI::TextureView* Canvas::GetTextureView() const
    {
        return textureView.Get();
    }

    void Canvas::Resize(uint32_t inWidth, uint32_t inHeight)
    {
        const uint32_t newWidth = Internal::ClampExtent(inWidth);
        const uint32_t newHeight = Internal::ClampExtent(inHeight);
        if (newWidth == width && newHeight == height) {
            return;
        }

        width = newWidth;
        height = newHeight;
        textureView.Reset();
        renderTargetView.Reset();
        texture.Reset();
        CreateTarget();
    }

    void Canvas::CreateTarget()
    {
        texture = device.CreateTexture(
            RHI::TextureCreateInfo()
                .SetDimension(RHI::TextureDimension::t2D)
                .SetWidth(width)
                .SetHeight(height)
                .SetDepthOrArraySize(1)
                .SetFormat(format)
                .SetUsages(RHI::TextureUsageBits::renderAttachment | RHI::TextureUsageBits::textureBinding)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(RHI::TextureState::shaderReadOnly)
                .SetDebugName(debugName));
        textureView = texture->CreateTextureView(
            RHI::TextureViewCreateInfo()
                .SetDimension(RHI::TextureViewDimension::tv2D)
                .SetMipLevels(0, 1)
                .SetArrayLayers(0, 1)
                .SetAspect(RHI::TextureAspect::color)
                .SetType(RHI::TextureViewType::textureBinding));
        renderTargetView = texture->CreateTextureView(
            RHI::TextureViewCreateInfo()
                .SetDimension(RHI::TextureViewDimension::tv2D)
                .SetMipLevels(0, 1)
                .SetArrayLayers(0, 1)
                .SetAspect(RHI::TextureAspect::color)
                .SetType(RHI::TextureViewType::colorAttachment));
    }
}

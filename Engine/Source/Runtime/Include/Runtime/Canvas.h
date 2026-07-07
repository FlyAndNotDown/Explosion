//
// Created by johnk on 2026/7/8.
//

#pragma once

#include <string>

#include <Common/Memory.h>
#include <RHI/RHI.h>
#include <Runtime/RenderSurface.h>

namespace Runtime {
    struct RUNTIME_API CanvasDesc {
        CanvasDesc();

        uint32_t width;
        uint32_t height;
        RHI::PixelFormat format;
        std::string debugName;
    };

    class RUNTIME_API Canvas : public RenderSurface {
    public:
        Canvas(RHI::Device& inDevice, const CanvasDesc& inDesc);
        ~Canvas() override;

        RHI::Texture* GetTexture() const override;
        RHI::TextureView* GetRenderTargetView() const override;
        RHI::TextureView* GetTextureView() const;
        void Resize(uint32_t inWidth, uint32_t inHeight) override;

    private:
        void CreateTarget();

        RHI::Device& device;
        uint32_t width;
        uint32_t height;
        RHI::PixelFormat format;
        std::string debugName;
        Common::UniquePtr<RHI::Texture> texture;
        Common::UniquePtr<RHI::TextureView> textureView;
        Common::UniquePtr<RHI::TextureView> renderTargetView;
    };
}

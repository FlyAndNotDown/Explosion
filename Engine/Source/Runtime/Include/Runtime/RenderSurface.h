//
// Created by johnk on 2026/7/8.
//

#pragma once

#include <cstdint>

#include <Common/Utility.h>
#include <RHI/Texture.h>
#include <Runtime/Api.h>

namespace Runtime {
    class RUNTIME_API RenderSurface {
    public:
        virtual ~RenderSurface();

        NonCopyable(RenderSurface)
        NonMovable(RenderSurface)

        virtual RHI::Texture* GetTexture() const = 0;
        virtual RHI::TextureView* GetRenderTargetView() const = 0;
        virtual void Resize(uint32_t inWidth, uint32_t inHeight) = 0;

    protected:
        RenderSurface();
    };
}

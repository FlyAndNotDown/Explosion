//
// Created by johnk on 2026/7/8.
//

#pragma once

#include <vector>

#include <Common/Memory.h>
#include <RHI/RHI.h>
#include <Runtime/RenderSurface.h>

namespace Runtime {
    class RUNTIME_API Window : public RenderSurface {
    public:
        ~Window() override;

        RHI::Texture* GetTexture() const override;
        RHI::TextureView* GetRenderTargetView() const override;
        void Resize(uint32_t inWidth, uint32_t inHeight) override;
        void AcquireBackTexture();
        void Present();
        RHI::Device& GetDevice() const;
        RHI::PixelFormat GetSwapChainFormat() const;
        RHI::TextureState GetCurrentBackTextureState() const;
        RHI::Semaphore* GetImageReadySemaphore() const;
        RHI::Semaphore* GetRenderFinishedSemaphore() const;
        void WaitRenderingIdle() const;

    protected:
        explicit Window(RHI::Device& inDevice);

        void InitializeSurface(Common::UniquePtr<RHI::Surface> inSurface, uint32_t inWidth, uint32_t inHeight);
        void DestroySurface();

    private:
        static constexpr uint8_t swapChainTextureNum = 2;

        void ReleaseSwapChainResources();
        void RecreateSwapChain(uint32_t inWidth, uint32_t inHeight);

        RHI::Device& device;
        Common::UniquePtr<RHI::Surface> surface;
        Common::UniquePtr<RHI::SwapChain> swapChain;
        std::vector<RHI::Texture*> swapChainTextures;
        std::vector<Common::UniquePtr<RHI::TextureView>> swapChainTextureViews;
        std::vector<RHI::TextureState> swapChainTextureStates;
        Common::UniquePtr<RHI::Semaphore> imageReadySemaphore;
        std::vector<Common::UniquePtr<RHI::Semaphore>> renderFinishedSemaphores;
        RHI::PixelFormat swapChainFormat;
        uint32_t currentBackTextureIndex;
        uint32_t width;
        uint32_t height;
    };
}

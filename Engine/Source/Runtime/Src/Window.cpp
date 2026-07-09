//
// Created by johnk on 2026/7/8.
//

#include <algorithm>
#include <optional>
#include <vector>

#include <Runtime/Window.h>

namespace Runtime::Internal {
    static uint32_t ClampExtent(uint32_t inValue)
    {
        return std::max(1u, inValue);
    }
}

namespace Runtime {
    Window::Window(RHI::Device& inDevice)
        : device(inDevice)
        , imageReadySemaphore((device.CreateSemaphore)())
        , swapChainFormat(RHI::PixelFormat::max)
        , currentBackTextureIndex(0)
        , width(1)
        , height(1)
    {
    }

    Window::~Window() = default;

    RHI::Texture* Window::GetTexture() const
    {
        Assert(currentBackTextureIndex < swapChainTextures.size());
        return swapChainTextures[currentBackTextureIndex];
    }

    RHI::TextureView* Window::GetRenderTargetView() const
    {
        Assert(currentBackTextureIndex < swapChainTextureViews.size());
        return swapChainTextureViews[currentBackTextureIndex].Get();
    }

    void Window::AcquireBackTexture()
    {
        currentBackTextureIndex = swapChain->AcquireBackTexture(imageReadySemaphore.Get());
        Assert(currentBackTextureIndex < swapChainTextures.size());
    }

    void Window::Resize(uint32_t inWidth, uint32_t inHeight)
    {
        RecreateSwapChain(Internal::ClampExtent(inWidth), Internal::ClampExtent(inHeight));
    }

    void Window::Present()
    {
        swapChain->Present(GetRenderFinishedSemaphore());
        swapChainTextureStates[currentBackTextureIndex] = RHI::TextureState::present;
    }

    RHI::Device& Window::GetDevice() const
    {
        return device;
    }

    RHI::PixelFormat Window::GetSwapChainFormat() const
    {
        return swapChainFormat;
    }

    RHI::TextureState Window::GetCurrentBackTextureState() const
    {
        Assert(currentBackTextureIndex < swapChainTextureStates.size());
        return swapChainTextureStates[currentBackTextureIndex];
    }

    RHI::Semaphore* Window::GetImageReadySemaphore() const
    {
        return imageReadySemaphore.Get();
    }

    RHI::Semaphore* Window::GetRenderFinishedSemaphore() const
    {
        Assert(currentBackTextureIndex < renderFinishedSemaphores.size());
        return renderFinishedSemaphores[currentBackTextureIndex].Get();
    }

    void Window::WaitRenderingIdle() const
    {
        const auto fence = device.CreateFence(false);
        device.GetQueue(RHI::QueueType::graphics, 0)->Flush(fence.Get());
        fence->Wait();
    }

    void Window::InitializeSurface(Common::UniquePtr<RHI::Surface> inSurface, uint32_t inWidth, uint32_t inHeight)
    {
        surface = std::move(inSurface);
        RecreateSwapChain(Internal::ClampExtent(inWidth), Internal::ClampExtent(inHeight));
    }

    void Window::DestroySurface()
    {
        ReleaseSwapChainResources();
        surface.Reset();
    }

    void Window::ReleaseSwapChainResources()
    {
        swapChainTextureViews.clear();
        swapChain.Reset();
        swapChainTextures.clear();
        swapChainTextureStates.clear();
        renderFinishedSemaphores.clear();
        currentBackTextureIndex = 0;
    }

    void Window::RecreateSwapChain(uint32_t inWidth, uint32_t inHeight)
    {
        static std::vector<RHI::PixelFormat> formatQualifiers = {
            RHI::PixelFormat::rgba8Unorm,
            RHI::PixelFormat::bgra8Unorm
        };

        ReleaseSwapChainResources();

        std::optional<RHI::PixelFormat> pixelFormat = {};
        for (const auto format : formatQualifiers) {
            if (device.CheckSwapChainFormatSupport(surface.Get(), format, RHI::ColorSpace::srgbNonLinear)) {
                pixelFormat = format;
                break;
            }
        }
        Assert(pixelFormat.has_value());
        swapChainFormat = pixelFormat.value();
        width = inWidth;
        height = inHeight;

        swapChain = device.CreateSwapChain(
            RHI::SwapChainCreateInfo()
                .SetPresentQueue(device.GetQueue(RHI::QueueType::graphics, 0))
                .SetSurface(surface.Get())
                .SetTextureNum(swapChainTextureNum)
                .SetFormat(swapChainFormat)
                .SetWidth(width)
                .SetHeight(height)
                .SetPresentMode(RHI::PresentMode::immediately));

        const auto actualTextureNum = swapChain->GetTextureNum();
        swapChainTextures.resize(actualTextureNum);
        swapChainTextureViews.resize(actualTextureNum);
        swapChainTextureStates.resize(actualTextureNum);
        renderFinishedSemaphores.resize(actualTextureNum);
        currentBackTextureIndex = 0;

        for (uint8_t i = 0; i < actualTextureNum; i++) {
            swapChainTextures[i] = swapChain->GetTexture(i);
            swapChainTextureStates[i] = swapChainTextures[i]->GetCreateInfo().initialState;
            swapChainTextureViews[i] = swapChainTextures[i]->CreateTextureView(
                RHI::TextureViewCreateInfo()
                    .SetDimension(RHI::TextureViewDimension::tv2D)
                    .SetMipLevels(0, 1)
                    .SetArrayLayers(0, 1)
                    .SetAspect(RHI::TextureAspect::color)
                    .SetType(RHI::TextureViewType::colorAttachment));
            renderFinishedSemaphores[i] = (device.CreateSemaphore)();
        }
    }
}

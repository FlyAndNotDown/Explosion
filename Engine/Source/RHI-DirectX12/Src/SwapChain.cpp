//
// Created by johnk on 28/3/2022.
//

#include <windows.h>
#include <dxgi1_6.h>

#include <RHI/DirectX12/Instance.h>
#include <RHI/DirectX12/Device.h>
#include <RHI/DirectX12/Gpu.h>
#include <RHI/DirectX12/SwapChain.h>
#include <RHI/DirectX12/Queue.h>
#include <RHI/DirectX12/Common.h>
#include <RHI/DirectX12/Texture.h>
#include <RHI/DirectX12/Surface.h>
#include <RHI/DirectX12/Synchronous.h>

namespace RHI::DirectX12 {
    // DXGI only exposes the vblank sync interval as a knob, so every vsynced mode (vsync/mailbox/fifoRelaxed) maps to 1 and
    // only immediately maps to 0. Tearing (true uncapped present) is then opted into separately via the ALLOW_TEARING flags.
    static uint8_t GetSyncInterval(const PresentMode presentMode)
    {
        return presentMode == PresentMode::immediately ? 0 : 1;
    }

    static bool CheckAllowTearingSupport(IDXGIFactory4* inFactory)
    {
        ComPtr<IDXGIFactory5> factory5;
        if (FAILED(inFactory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
            return false;
        }
        BOOL allowTearing = FALSE;
        return SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))
            && allowTearing == TRUE;
    }
}

namespace RHI::DirectX12 {
    DX12SwapChain::DX12SwapChain(DX12Device& inDevice, const SwapChainCreateInfo& inCreateInfo)
        : SwapChain(inCreateInfo)
        , device(inDevice)
        , queue(static_cast<DX12Queue&>(*inCreateInfo.presentQueue))
        , textureNum(inCreateInfo.textureNum)
        , presentMode(inCreateInfo.presentMode)
        , allowTearing(false)
    {
        CreateDX12SwapChain(inCreateInfo);
        FetchTextures(inCreateInfo);
    }

    DX12SwapChain::~DX12SwapChain() = default;

    uint8_t DX12SwapChain::GetTextureNum()
    {
        return textureNum;
    }

    Texture* DX12SwapChain::GetTexture(uint8_t inIndex)
    {
        return textures[inIndex].Get();
    }

    uint8_t DX12SwapChain::AcquireBackTexture(Semaphore* inSignalSemaphore)
    {
        auto& dx12SignalSemaphore = static_cast<DX12Semaphore&>(*inSignalSemaphore);
        const auto result = static_cast<uint8_t>(nativeSwapChain->GetCurrentBackBufferIndex());
        auto* fence = dx12SignalSemaphore.GetNative();
        Assert(SUCCEEDED(fence->Signal(0)));
        Assert(SUCCEEDED(fence->Signal(1)));
        return result;
    }

    void DX12SwapChain::Present(Semaphore* inWaitSemaphore)
    {
        auto& dx12WaitSemaphore = static_cast<DX12Semaphore&>(*inWaitSemaphore);
        Assert(SUCCEEDED(queue.GetNative()->Wait(dx12WaitSemaphore.GetNative(), 1)));

        const auto syncInterval = GetSyncInterval(presentMode);
        // DXGI_PRESENT_ALLOW_TEARING is only legal with a sync interval of 0 and an ALLOW_TEARING swap chain.
        const UINT presentFlags = syncInterval == 0 && allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
        Assert(SUCCEEDED(nativeSwapChain->Present(syncInterval, presentFlags)));
    }

    void DX12SwapChain::CreateDX12SwapChain(const SwapChainCreateInfo& inCreateInfo)
    {
        const auto& instance = device.GetGpu().GetInstance();
        const auto* dx12Queue = static_cast<DX12Queue*>(inCreateInfo.presentQueue);
        Assert(dx12Queue != nullptr);
        const auto* dx12Surface = static_cast<DX12Surface*>(inCreateInfo.surface);
        Assert(dx12Surface != nullptr);

        // Tearing (uncapped, IMMEDIATE-like present) needs both the ALLOW_TEARING swap-chain flag here and the matching
        // present flag later, and is only usable when the DXGI factory reports support for it.
        allowTearing = inCreateInfo.presentMode == PresentMode::immediately && CheckAllowTearingSupport(instance.GetNative());

        DXGI_SWAP_CHAIN_DESC1 desc {};
        desc.BufferCount = inCreateInfo.textureNum;
        desc.Width = inCreateInfo.width;
        desc.Height = inCreateInfo.height;
        desc.Format = EnumCast<PixelFormat, DXGI_FORMAT>(inCreateInfo.format);
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc.Count = 1;
        desc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> dx12SwapChain1;
        bool success = SUCCEEDED(instance.GetNative()->CreateSwapChainForHwnd(
            dx12Queue->GetNative(),
            dx12Surface->GetNative(),
            &desc,
            nullptr,
            nullptr,
            &dx12SwapChain1));
        Assert(success);

        success = SUCCEEDED(dx12SwapChain1.As(&nativeSwapChain));
        Assert(success);

        const auto dxgiColorSpace = EnumCast<ColorSpace, DXGI_COLOR_SPACE_TYPE>(inCreateInfo.colorSpace);
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(nativeSwapChain->CheckColorSpaceSupport(dxgiColorSpace, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
            Assert(SUCCEEDED(nativeSwapChain->SetColorSpace1(dxgiColorSpace)));
        }
    }

    void DX12SwapChain::FetchTextures(const SwapChainCreateInfo& inCreateInfo)
    {
        textures.resize(textureNum);
        for (auto i = 0; i < textureNum; i++) {
            ComPtr<ID3D12Resource> dx12Resource;
            Assert(SUCCEEDED(nativeSwapChain->GetBuffer(i, IID_PPV_ARGS(&dx12Resource))));

            TextureCreateInfo textureCreateInfo = TextureCreateInfo()
                .SetDimension(TextureDimension::t2D)
                .SetWidth(inCreateInfo.width)
                .SetHeight(inCreateInfo.height)
                .SetDepthOrArraySize(1)
                .SetFormat(inCreateInfo.format)
                .SetUsages(TextureUsageBits::renderAttachment)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(TextureState::present)
                .SetDebugName(std::string("BackBuffer-") + std::to_string(i));
            textures[i] = new DX12Texture(device, textureCreateInfo, std::move(dx12Resource));
        }
    }
}

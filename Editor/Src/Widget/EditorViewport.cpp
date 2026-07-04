//
// Created by johnk on 2026/7/4.
//

#include <algorithm>

#include <QResizeEvent>

#include <Editor/Widget/EditorViewport.h>
#include <Editor/Widget/moc_EditorViewport.cpp> // NOLINT
#include <Runtime/Engine.h>

// unity builds can pull windows.h (via httplib/qt) into this translation unit after the RHI headers already ran
// their cleanup, so the win32 semaphore macro would mangle RHI::Device::CreateSemaphore below
#undef CreateSemaphore

namespace Editor {
    EditorViewport::EditorViewport(EditorClient& inClient, QWidget* inParent)
        : GraphicsWidget(inParent)
        , client(inClient)
        , imageReadySemaphore(GetDevice().CreateSemaphore())
        , renderFinishedSemaphore(GetDevice().CreateSemaphore())
    {
        setMinimumSize(320, 240);
        setFocusPolicy(Qt::StrongFocus);
        RecreateSwapChain(GetWidth(), GetHeight());
        client.SetViewport(this);
    }

    EditorViewport::~EditorViewport()
    {
        client.SetViewport(nullptr);
        WaitRenderingIdle();
        swapChain.Reset();
    }

    Runtime::Client& EditorViewport::GetClient()
    {
        return client;
    }

    Runtime::PresentInfo EditorViewport::GetNextPresentInfo()
    {
        const auto backTextureIndex = swapChain->AcquireBackTexture(imageReadySemaphore.Get());

        Runtime::PresentInfo result;
        result.backTexture = swapChain->GetTexture(backTextureIndex);
        result.imageReadySemaphore = imageReadySemaphore.Get();
        result.renderFinishedSemaphore = renderFinishedSemaphore.Get();
        return result;
    }

    uint32_t EditorViewport::GetWidth() const
    {
        return static_cast<uint32_t>(std::max(width(), 1));
    }

    uint32_t EditorViewport::GetHeight() const
    {
        return static_cast<uint32_t>(std::max(height(), 1));
    }

    void EditorViewport::Resize(uint32_t inWidth, uint32_t inHeight)
    {
        RecreateSwapChain(inWidth, inHeight);
    }

    void EditorViewport::Present(const Runtime::PresentInfo& inPresentInfo)
    {
        swapChain->Present(inPresentInfo.renderFinishedSemaphore);
    }

    void EditorViewport::resizeEvent(QResizeEvent* inEvent)
    {
        GraphicsWidget::resizeEvent(inEvent);
        if (const QSize size = inEvent->size();
            swapChain.Valid() && size.width() > 0 && size.height() > 0) {
            RecreateSwapChain(static_cast<uint32_t>(size.width()), static_cast<uint32_t>(size.height()));
        }
    }

    bool EditorViewport::event(QEvent* inEvent)
    {
        // docking/floating the widget can recreate its native window, the surface and swapchain bound to the old
        // window handle must be rebuilt on the new one
        if (inEvent->type() == QEvent::WinIdChange && swapChain.Valid()) {
            RecreateSurfaceAndSwapChain();
        }
        return GraphicsWidget::event(inEvent);
    }

    void EditorViewport::RecreateSwapChain(uint32_t inWidth, uint32_t inHeight)
    {
        static std::vector<RHI::PixelFormat> formatQualifiers = {
            RHI::PixelFormat::rgba8Unorm,
            RHI::PixelFormat::bgra8Unorm
        };

        WaitRenderingIdle();
        if (swapChain.Valid()) {
            swapChain.Reset();
        }

        std::optional<RHI::PixelFormat> pixelFormat = {};
        for (const auto format : formatQualifiers) {
            if (GetDevice().CheckSwapChainFormatSupport(&GetSurface(), format, RHI::ColorSpace::srgbNonLinear)) {
                pixelFormat = format;
                break;
            }
        }
        Assert(pixelFormat.has_value());

        swapChain = GetDevice().CreateSwapChain(
            RHI::SwapChainCreateInfo()
                .SetPresentQueue(GetDevice().GetQueue(RHI::QueueType::graphics, 0))
                .SetSurface(&GetSurface())
                .SetTextureNum(2)
                .SetFormat(pixelFormat.value())
                .SetWidth(inWidth)
                .SetHeight(inHeight)
                .SetPresentMode(RHI::PresentMode::immediately));
    }

    void EditorViewport::RecreateSurfaceAndSwapChain()
    {
        WaitRenderingIdle();
        swapChain.Reset();
        surface = device->CreateSurface(
            RHI::SurfaceCreateInfo()
                .SetWindow(reinterpret_cast<void*>(winId()))); // NOLINT
        RecreateSwapChain(GetWidth(), GetHeight());
    }

    void EditorViewport::WaitRenderingIdle() const
    {
        Runtime::EngineHolder::Get().GetRenderModule().GetRenderThread().Flush();
        WaitDeviceIdle();
    }
}

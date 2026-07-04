//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <Editor/EditorClient.h>
#include <Editor/Widget/GraphicsWidget.h>
#include <Runtime/Viewport.h>

namespace Editor {
    // qt widget hosting the engine-rendered scene: implements Runtime::Viewport over a swapchain created on the
    // widget's native window, RenderSystem acquires/presents through this seam every engine tick
    class EditorViewport final : public GraphicsWidget, public Runtime::Viewport {
        Q_OBJECT

    public:
        explicit EditorViewport(EditorClient& inClient, QWidget* inParent = nullptr);
        ~EditorViewport() override;

        Runtime::Client& GetClient() override;
        Runtime::PresentInfo GetNextPresentInfo() override;
        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;
        void Resize(uint32_t inWidth, uint32_t inHeight) override;
        void Present(const Runtime::PresentInfo& inPresentInfo) override;

    protected:
        void resizeEvent(QResizeEvent* inEvent) override;
        bool event(QEvent* inEvent) override;

    private:
        void RecreateSwapChain(uint32_t inWidth, uint32_t inHeight);
        void RecreateSurfaceAndSwapChain();
        void WaitRenderingIdle() const;

        EditorClient& client;
        Common::UniquePtr<RHI::Semaphore> imageReadySemaphore;
        Common::UniquePtr<RHI::Semaphore> renderFinishedSemaphore;
        Common::UniquePtr<RHI::SwapChain> swapChain;
    };
}

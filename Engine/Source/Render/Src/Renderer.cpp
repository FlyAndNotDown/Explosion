//
// Created by johnk on 2022/8/3.
//

#include <Render/Renderer.h>

namespace Render::Internal {
    const Common::LinearColor surfaceClearColor = { 0.1f, 0.1f, 0.12f, 1.0f };
}

namespace Render {
    Renderer::Renderer(const Params& inParams)
        : device(inParams.device)
        , scene(inParams.scene)
        , surface(inParams.surface)
        , surfaceExtent(inParams.surfaceExtent)
        , views(inParams.views)
        , waitSemaphore(inParams.waitSemaphore)
        , signalSemaphore(inParams.signalSemaphore)
        , signalFence(inParams.signalFence)
    {
    }

    Renderer::~Renderer() = default;

    StandardRenderer::StandardRenderer(const Params& inParams)
        : Renderer(inParams)
        , rgBuilder(*device)
    {
    }

    StandardRenderer::~StandardRenderer() = default;

    void StandardRenderer::Render(float inDeltaTimeSeconds)
    {
        auto* backTexture = rgBuilder.ImportTexture(surface, RHI::TextureState::present);
        auto* backTextureView = rgBuilder.CreateTextureView(backTexture, RGTextureViewDesc(RHI::TextureViewType::colorAttachment, RHI::TextureViewDimension::tv2D));

        rgBuilder.AddRasterPass(
            "BasePass",
            RGRasterPassDesc()
                .AddColorAttachment(RGColorAttachment(backTextureView, RHI::LoadOp::clear, RHI::StoreOp::store, Internal::surfaceClearColor)),
            {},
            [](const RGBuilder&, RHI::RasterPassCommandRecorder&) -> void {},
            {},
            [backTexture](const RGBuilder& rg, RHI::CommandRecorder& recorder) -> void {
                recorder.ResourceBarrier(RHI::Barrier::Transition(rg.GetRHI(backTexture), RHI::TextureState::renderTarget, RHI::TextureState::present));
            });

        RGExecuteInfo executeInfo;
        executeInfo.semaphoresToWait = { waitSemaphore };
        executeInfo.semaphoresToSignal = { signalSemaphore };
        executeInfo.inFenceToSignal = signalFence;
        rgBuilder.Execute(executeInfo);

        FinalizeViews();
    }

    void StandardRenderer::FinalizeViews() const
    {
        for (const auto& view : views) {
            view.state->prevData = view.data;
        }
    }
}

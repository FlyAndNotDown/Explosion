//
// Created by johnk on 2025/3/4.
//

#include <Common/Math/Projection.h>
#include <Render/Renderer.h>
#include <Runtime/Component/Camera.h>
#include <Runtime/Component/Player.h>
#include <Runtime/Component/Scene.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/Engine.h>
#include <Runtime/System/Render.h>

namespace Runtime {
    RenderSystem::RenderSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext)
        : System(inRegistry, inContext)
        , renderModule(EngineHolder::Get().GetRenderModule())
        , client(inContext.client)
        , lastFrameFence(renderModule.GetDevice()->CreateFence(true).Release())
    {
    }

    RenderSystem::~RenderSystem() // NOLINT
    {
        renderModule.GetRenderThread().EmplaceTask([fence = lastFrameFence]() -> void {
            fence->Wait();
            delete fence;
        });
    }

    void RenderSystem::Tick(float inDeltaTimeSeconds)
    {
        auto* clientViewport = client != nullptr ? client->GetViewport() : nullptr;
        if (clientViewport == nullptr) {
            return;
        }

        renderModule.GetRenderThread().EmplaceTask(
            [
                fence = lastFrameFence,
                views = BuildViews(),
                scene = registry.GGet<SceneHolder>().scene.Get(),
                surfaceExtent = Common::UVec2(clientViewport->GetWidth(), clientViewport->GetHeight()),
                viewport = clientViewport,
                renderModule = &renderModule,
                inDeltaTimeSeconds
            ]() -> void {
                fence->Reset();
                const auto presentInfo = viewport->GetNextPresentInfo();

                Render::StandardRenderer::Params rendererParams;
                rendererParams.device = renderModule->GetDevice();
                rendererParams.scene = scene;
                rendererParams.surface = presentInfo.backTexture;
                rendererParams.surfaceExtent = surfaceExtent;
                rendererParams.views = views;
                rendererParams.waitSemaphore = presentInfo.imageReadySemaphore;
                rendererParams.signalSemaphore = presentInfo.renderFinishedSemaphore;
                rendererParams.signalFence = fence;

                auto renderer = renderModule->CreateStandardRenderer(rendererParams);
                renderer.Render(inDeltaTimeSeconds);
                viewport->Present(presentInfo);
                // this frame's gpu work must finish before the renderer (and its recorded command buffers /
                // transient resources) goes out of scope, and it keeps the shared semaphores safe to reuse
                fence->Wait();
            });
    }

    Common::URect RenderSystem::GetPlayerViewport(uint32_t inWidth, uint32_t inHeight, uint8_t inPlayerNum, uint8_t inPlayerIndex)
    {
        Assert(inPlayerNum > 0 && inPlayerIndex < inPlayerNum);
        if (inPlayerNum == 1) {
            return { 0, 0, inWidth, inHeight };
        }
        if (inPlayerNum == 2) {
            const auto widthPerPlayer = inWidth / 2;
            return { widthPerPlayer * inPlayerIndex, 0, widthPerPlayer, inHeight };
        }
        if (inPlayerNum <= 4) {
            const auto widthPerPlayer = inWidth / 2;
            const auto heightPerPlayer = inHeight / 2;
            return { widthPerPlayer * (inPlayerIndex % 2), heightPerPlayer * (inPlayerIndex / 2), widthPerPlayer, heightPerPlayer };
        }
        Unimplement();
        return {};
    }

    Render::View RenderSystem::BuildViewForCamera(Entity inEntity) const
    {
        auto& camera = registry.Get<Camera>(inEntity);
        const auto& worldTransform = registry.Get<WorldTransform>(inEntity);
        const auto& clientViewport = *client->GetViewport();

        const auto width = clientViewport.GetWidth();
        const auto height = clientViewport.GetHeight();

        if (camera.viewState == nullptr) {
            camera.viewState = renderModule.NewViewState();
        }

        // a player camera renders into its split-screen sub-viewport, any other camera (e.g. the editor camera)
        // covers the whole client viewport
        Common::URect viewportRect = { 0, 0, width, height };
        if (registry.Has<LocalPlayer>(inEntity)) {
            const auto& playersInfo = registry.GGet<PlayersInfo>();
            viewportRect = GetPlayerViewport(width, height, static_cast<uint8_t>(playersInfo.players.size()), registry.Get<LocalPlayer>(inEntity).localPlayerIndex);
        }

        Render::View view = renderModule.CreateView();
        view.state = camera.viewState.Get();
        view.data.viewport = viewportRect;
        view.data.viewMatrix = worldTransform.localToWorld.GetTransformMatrixNoScale().Inverse();
        view.data.origin = worldTransform.localToWorld.translation;
        if (camera.perspective) {
            const Common::FReversedZPerspectiveProjection projection(camera.fov.value(), static_cast<float>(width), static_cast<float>(height), camera.nearPlane, camera.farPlane);
            view.data.projectionMatrix = projection.GetProjectionMatrix();
        } else {
            const Common::FReversedZOrthoProjection projection(static_cast<float>(width), static_cast<float>(height), camera.nearPlane, camera.farPlane);
            view.data.projectionMatrix = projection.GetProjectionMatrix();
        }
        return view;
    }

    std::vector<Render::View> RenderSystem::BuildViews() const
    {
        const auto cameras = registry.View<Camera, WorldTransform>().All();

        std::vector<Render::View> result;
        result.reserve(cameras.size());
        for (const auto& camera : cameras) {
            result.emplace_back(BuildViewForCamera(std::get<0>(camera)));
        }
        return result;
    }
}

//
// Created by johnk on 2025/3/4.
//

#include <Common/Math/Projection.h>
#include <Common/Math/View.h>
#include <Render/Renderer.h>
#include <Runtime/Component/Camera.h>
#include <Runtime/Component/Player.h>
#include <Runtime/Component/Scene.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/Engine.h>
#include <Runtime/System/Render.h>
#include <Runtime/Window.h>

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
        auto* target = client != nullptr ? client->GetRenderSurface() : nullptr;
        if (target == nullptr) {
            return;
        }
        auto* window = dynamic_cast<Window*>(target);

        auto* texture = target->GetTexture();
        Assert(texture != nullptr);
        const auto& textureDesc = texture->GetCreateInfo();

        renderModule.GetRenderThread().EmplaceTask(
            [
                fence = lastFrameFence,
                views = BuildViews(),
                scene = registry.GGet<SceneHolder>().scene.Get(),
                surfaceExtent = Common::UVec2(textureDesc.width, textureDesc.height),
                target,
                window,
                renderModule = &renderModule,
                inDeltaTimeSeconds
            ]() -> void {
                fence->Reset();
                if (window != nullptr) {
                    window->AcquireBackTexture();
                }

                Render::StandardRenderer::Params rendererParams;
                rendererParams.device = renderModule->GetDevice();
                rendererParams.scene = scene;
                rendererParams.surface = target->GetTexture();
                rendererParams.surfaceExtent = surfaceExtent;
                rendererParams.surfaceBeforeRenderState = window != nullptr
                    ? window->GetCurrentBackTextureState()
                    : RHI::TextureState::shaderReadOnly;
                rendererParams.surfaceAfterRenderState = window != nullptr
                    ? RHI::TextureState::present
                    : RHI::TextureState::shaderReadOnly;
                rendererParams.views = views;
                rendererParams.waitSemaphore = window != nullptr ? window->GetImageReadySemaphore() : nullptr;
                rendererParams.signalSemaphore = window != nullptr ? window->GetRenderFinishedSemaphore() : nullptr;
                rendererParams.signalFence = fence;

                auto renderer = renderModule->CreateStandardRenderer(rendererParams);
                renderer.Render(inDeltaTimeSeconds);
                if (window != nullptr) {
                    window->Present();
                }
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
        auto* target = client->GetRenderSurface();
        Assert(target != nullptr && target->GetTexture() != nullptr);
        const auto& textureDesc = target->GetTexture()->GetCreateInfo();
        const auto width = textureDesc.width;
        const auto height = textureDesc.height;

        if (camera.viewState == nullptr) {
            camera.viewState = renderModule.NewViewState();
        }

        // a player camera renders into its split-screen sub-viewport, any other camera (e.g. the editor camera)
        // covers the whole render surface
        Common::URect viewportRect = { 0, 0, width, height };
        if (registry.Has<LocalPlayer>(inEntity)) {
            const auto& playersInfo = registry.GGet<PlayersInfo>();
            viewportRect = GetPlayerViewport(width, height, static_cast<uint8_t>(playersInfo.players.size()), registry.Get<LocalPlayer>(inEntity).localPlayerIndex);
        }

        Render::View view = renderModule.CreateView();
        view.state = camera.viewState.Get();
        view.data.viewport = viewportRect;
        view.data.viewMatrix = Common::FViewTransform(worldTransform.localToWorld).GetViewMatrix();
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

//
// Created by johnk on 2025/1/9.
//

#include <unordered_set>
#include <utility>

#include <Runtime/System/Scene.h>
#include <Runtime/Engine.h>
#include <Runtime/Component/Transform.h>

namespace Runtime {
    SceneSystem::SceneSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext)
        : System(inRegistry, inContext)
        , renderModule(EngineHolder::Get().GetRenderModule())
        , transformUpdatedObserver(inRegistry.Observer())
        , directionalLightsObserver(inRegistry.EventsObserver<DirectionalLight>())
        , pointLightsObserver(inRegistry.EventsObserver<PointLight>())
        , spotLightsObserver(inRegistry.EventsObserver<SpotLight>())
        , staticPrimitivesObserver(inRegistry.EventsObserver<StaticPrimitive>())
    {
        transformUpdatedObserver
            .ObConstructed<WorldTransform>()
            .ObUpdated<WorldTransform>();

        inRegistry.GEmplace<SceneHolder>(renderModule.NewScene());

        inRegistry.View<DirectionalLight>().Each([this](Entity e, DirectionalLight&) -> void { QueueCreateSceneProxy<DirectionalLight, Render::DirectionalLightSceneProxy>(e); });
        inRegistry.View<PointLight>().Each([this](Entity e, PointLight&) -> void { QueueCreateSceneProxy<PointLight, Render::PointLightSceneProxy>(e); });
        inRegistry.View<SpotLight>().Each([this](Entity e, SpotLight&) -> void { QueueCreateSceneProxy<SpotLight, Render::SpotLightSceneProxy>(e); });
        inRegistry.View<StaticPrimitive>().Each([this](Entity e, StaticPrimitive&) -> void { QueueCreateSceneProxy<StaticPrimitive, Render::StaticPrimitiveSceneProxy>(e, true); });
    }

    SceneSystem::~SceneSystem() // NOLINT
    {
        registry.GRemove<SceneHolder>();
    }

    void SceneSystem::Tick(float inDeltaTimeSeconds)
    {
        ProcessSceneProxyEvents<DirectionalLight, Render::DirectionalLightSceneProxy>(directionalLightsObserver);
        ProcessSceneProxyEvents<PointLight, Render::PointLightSceneProxy>(pointLightsObserver);
        ProcessSceneProxyEvents<SpotLight, Render::SpotLightSceneProxy>(spotLightsObserver);
        ProcessSceneProxyEvents<StaticPrimitive, Render::StaticPrimitiveSceneProxy>(staticPrimitivesObserver, true);

        std::unordered_set<Entity> changedWorldTransforms;
        transformUpdatedObserver.Each([&](Entity e) -> void { changedWorldTransforms.emplace(e); });
        for (const Entity e : changedWorldTransforms) {
            if (!registry.Valid(e) || !registry.Has<WorldTransform>(e)) {
                continue;
            }
            if (registry.Has<DirectionalLight>(e)) {
                QueueUpdateSceneProxyTransform<Render::DirectionalLightSceneProxy>(e);
            }
            if (registry.Has<PointLight>(e)) {
                QueueUpdateSceneProxyTransform<Render::PointLightSceneProxy>(e);
            }
            if (registry.Has<SpotLight>(e)) {
                QueueUpdateSceneProxyTransform<Render::SpotLightSceneProxy>(e);
            }
            if (registry.Has<StaticPrimitive>(e)) {
                QueueUpdateSceneProxyTransform<Render::StaticPrimitiveSceneProxy>(e, true);
            }
        }

        transformUpdatedObserver.Clear();
    }
}

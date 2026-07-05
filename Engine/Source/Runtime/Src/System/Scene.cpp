//
// Created by johnk on 2025/1/9.
//

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

        // components created before the system graph was built (e.g. a loaded level or editor-authored defaults)
        // never fire construct events, mirror them into the render scene here
        inRegistry.View<DirectionalLight>().Each([this](Entity e, DirectionalLight&) -> void { QueueCreateSceneProxy<DirectionalLight, Render::LightSceneProxy>(e); });
        inRegistry.View<PointLight>().Each([this](Entity e, PointLight&) -> void { QueueCreateSceneProxy<PointLight, Render::LightSceneProxy>(e); });
        inRegistry.View<SpotLight>().Each([this](Entity e, SpotLight&) -> void { QueueCreateSceneProxy<SpotLight, Render::LightSceneProxy>(e); });
        inRegistry.View<StaticPrimitive>().Each([this](Entity e, StaticPrimitive&) -> void { QueueCreateSceneProxy<StaticPrimitive, Render::PrimitiveSceneProxy>(e, true); });
    }

    SceneSystem::~SceneSystem() // NOLINT
    {
        registry.GRemove<SceneHolder>();
    }

    void SceneSystem::Tick(float inDeltaTimeSeconds)
    {
        directionalLightsObserver.Constructed().Each([this](Entity e) -> void { QueueCreateSceneProxy<DirectionalLight, Render::LightSceneProxy>(e); });
        pointLightsObserver.Constructed().Each([this](Entity e) -> void { QueueCreateSceneProxy<PointLight, Render::LightSceneProxy>(e); });
        spotLightsObserver.Constructed().Each([this](Entity e) -> void { QueueCreateSceneProxy<SpotLight, Render::LightSceneProxy>(e); });
        staticPrimitivesObserver.Constructed().Each([this](Entity e) -> void { QueueCreateSceneProxy<StaticPrimitive, Render::PrimitiveSceneProxy>(e, true); });
        directionalLightsObserver.Updated().Each([this](Entity e) -> void { QueueUpdateSceneProxyContent<DirectionalLight, Render::LightSceneProxy>(e); });
        pointLightsObserver.Updated().Each([this](Entity e) -> void { QueueUpdateSceneProxyContent<PointLight, Render::LightSceneProxy>(e); });
        spotLightsObserver.Updated().Each([this](Entity e) -> void { QueueUpdateSceneProxyContent<SpotLight, Render::LightSceneProxy>(e); });
        staticPrimitivesObserver.Updated().Each([this](Entity e) -> void { QueueUpdateSceneProxyContent<StaticPrimitive, Render::PrimitiveSceneProxy>(e); });
        directionalLightsObserver.Removed().Each([this](Entity e) -> void { QueueRemoveSceneProxy<Render::LightSceneProxy>(e); });
        pointLightsObserver.Removed().Each([this](Entity e) -> void { QueueRemoveSceneProxy<Render::LightSceneProxy>(e); });
        spotLightsObserver.Removed().Each([this](Entity e) -> void { QueueRemoveSceneProxy<Render::LightSceneProxy>(e); });
        staticPrimitivesObserver.Removed().Each([this](Entity e) -> void { QueueRemoveSceneProxy<Render::PrimitiveSceneProxy>(e); });

        transformUpdatedObserver.Each([this](Entity e) -> void {
            if (registry.Has<DirectionalLight>(e) || registry.Has<PointLight>(e) || registry.Has<SpotLight>(e)) {
                QueueUpdateSceneProxyTransform<Render::LightSceneProxy>(e);
            }
            if (registry.Has<StaticPrimitive>(e)) {
                QueueUpdateSceneProxyTransform<Render::PrimitiveSceneProxy>(e, true);
            }
        });

        transformUpdatedObserver.Clear();
        directionalLightsObserver.Clear();
        pointLightsObserver.Clear();
        spotLightsObserver.Clear();
        staticPrimitivesObserver.Clear();
    }
}

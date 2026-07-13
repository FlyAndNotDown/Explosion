//
// Created by johnk on 2025/1/9.
//

#pragma once

#include <optional>

#include <Runtime/ECS.h>
#include <Runtime/Component/Light.h>
#include <Runtime/Component/Primitive.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/Component/Scene.h>
#include <Runtime/Engine.h>
#include <Render/MeshRenderData.h>
#include <Render/RenderModule.h>
#include <Render/Scene.h>
#include <Render/SceneProxy/Light.h>
#include <Render/SceneProxy/Primitive.h>
#include <Render/VertexFactory.h>

namespace Runtime {
    class RUNTIME_API EClass() SceneSystem final : public System {
        EPolyDerivedClassBody(SceneSystem)

    public:
        explicit SceneSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext);
        ~SceneSystem() override;

        NonCopyable(SceneSystem)
        NonMovable(SceneSystem)

        void Tick(float inDeltaTimeSeconds) override;

    private:
        template <typename Component, typename SceneProxy> void QueueCreateSceneProxy(Entity inEntity, bool inWithScale = false);
        template <typename Component, typename SceneProxy> void QueueUpdateSceneProxyContent(Entity inEntity);
        template <typename SceneProxy> void QueueUpdateSceneProxyTransform(Entity inEntity, bool inWithScale = false);
        template <typename SceneProxy> void QueueRemoveSceneProxy(Entity inEntity);

        Render::RenderModule& renderModule;
        Observer transformUpdatedObserver;
        EventsObserver<DirectionalLight> directionalLightsObserver;
        EventsObserver<PointLight> pointLightsObserver;
        EventsObserver<SpotLight> spotLightsObserver;
        EventsObserver<StaticPrimitive> staticPrimitivesObserver;
    };
}

namespace Runtime::Internal {
    template <typename Component, typename SceneProxy>
    static void UpdateSceneProxyContent(SceneProxy& outSceneProxy, const Component& inComponent)
    {
        Unimplement();
    }

    template <typename SceneProxy>
    static void UpdateSceneProxyWorldTransform(SceneProxy& outSceneProxy, const WorldTransform& inTransform, bool withScale = true)
    {
        outSceneProxy.localToWorld = withScale ? inTransform.localToWorld.GetTransformMatrix() : inTransform.localToWorld.GetTransformMatrixNoScale();
    }

    template <typename T>
    static std::optional<T> GetOptional(const T* inObj)
    {
        return inObj == nullptr ? std::optional<T>() : *inObj;
    }
}

namespace Runtime::Internal {
    template <>
    inline void UpdateSceneProxyContent<DirectionalLight, Render::LightSceneProxy>(Render::LightSceneProxy& outSceneProxy, const DirectionalLight& inComponent)
    {
        outSceneProxy.type = Render::LightType::directional;
        outSceneProxy.color = inComponent.color;
        outSceneProxy.intensity = inComponent.intensity;
    }

    template <>
    inline void UpdateSceneProxyContent<PointLight, Render::LightSceneProxy>(Render::LightSceneProxy& outSceneProxy, const PointLight& inComponent)
    {
        outSceneProxy.type = Render::LightType::point;
        outSceneProxy.color = inComponent.color;
        outSceneProxy.intensity = inComponent.intensity;
        outSceneProxy.radius = inComponent.radius;
    }

    template <>
    inline void UpdateSceneProxyContent<SpotLight, Render::LightSceneProxy>(Render::LightSceneProxy& outSceneProxy, const SpotLight& inComponent)
    {
        outSceneProxy.type = Render::LightType::spot;
        outSceneProxy.color = inComponent.color;
        outSceneProxy.intensity = inComponent.intensity;
    }

    // runs on the render thread: uploads the mesh geometry and resolves the material's shader types, the asset chain
    // is kept alive by the component copy captured into the render thread task
    template <>
    inline void UpdateSceneProxyContent<StaticPrimitive, Render::PrimitiveSceneProxy>(Render::PrimitiveSceneProxy& outSceneProxy, const StaticPrimitive& inComponent)
    {
        outSceneProxy.mesh = nullptr;
        outSceneProxy.vertexFactoryType = nullptr;
        outSceneProxy.vertexShaderType = nullptr;
        outSceneProxy.pixelShaderType = nullptr;

        if (inComponent.mesh.Get() == nullptr || inComponent.mesh->GetLODCount() == 0) {
            return;
        }
        const AssetPtr<MaterialInstance>& materialInstance = inComponent.mesh->GetMaterial();
        if (materialInstance.Get() == nullptr || materialInstance->GetMaterial().Get() == nullptr) {
            return;
        }

        const StaticMeshVertices& vertices = inComponent.mesh->GetLOD(0).vertices;
        if (vertices.positions.empty() || vertices.indices.empty()) {
            return;
        }
        std::vector<Render::MeshRenderData::Vertex> gpuVertices;
        gpuVertices.reserve(vertices.positions.size());
        for (size_t i = 0; i < vertices.positions.size(); i++) {
            Render::MeshRenderData::Vertex vertex;
            vertex.position = vertices.positions[i];
            vertex.uv0 = i < vertices.uv0.size() ? vertices.uv0[i] : Common::FVec2();
            gpuVertices.emplace_back(vertex);
        }

        RHI::Device* device = EngineHolder::Get().GetRenderModule().GetDevice();
        outSceneProxy.mesh = new Render::MeshRenderData(*device, gpuVertices, vertices.indices);

        const Render::VertexFactoryType& vertexFactoryType = Render::StaticMeshVertexFactory::Get();
        const Material* material = materialInstance->GetMaterial().Get();
        outSceneProxy.vertexFactoryType = &vertexFactoryType;
        outSceneProxy.vertexShaderType = material->FindShaderType(vertexFactoryType.GetKey(), RHI::ShaderStageBits::sVertex);
        outSceneProxy.pixelShaderType = material->FindShaderType(vertexFactoryType.GetKey(), RHI::ShaderStageBits::sPixel);

        if (materialInstance->HasParameter("baseColor")) {
            if (const auto baseColorParam = materialInstance->GetParameter("baseColor");
                std::holds_alternative<Common::FVec4>(baseColorParam)) {
                outSceneProxy.baseColor = std::get<Common::FVec4>(baseColorParam);
            }
        }
    }
}

namespace Runtime {
    template <typename Component, typename SceneProxy>
    void SceneSystem::QueueCreateSceneProxy(Entity inEntity, bool inWithScale)
    {
        const auto& sceneHolder = registry.GGet<SceneHolder>();
        const auto& component = registry.Get<Component>(inEntity);
        const auto* transform = registry.Find<WorldTransform>(inEntity);
        renderModule.GetRenderThread().EmplaceTask([scene = sceneHolder.scene.Get(), inEntity, component, transform = Internal::GetOptional(transform), inWithScale]() -> void {
            SceneProxy sceneProxy;
            Internal::UpdateSceneProxyContent(sceneProxy, component);
            if (transform.has_value()) {
                Internal::UpdateSceneProxyWorldTransform(sceneProxy, transform.value(), inWithScale);
            }
            scene->Add<SceneProxy>(inEntity, std::move(sceneProxy));
        });
    }

    template <typename Component, typename SceneProxy>
    void SceneSystem::QueueUpdateSceneProxyContent(Entity inEntity)
    {
        const auto& sceneHolder = registry.GGet<SceneHolder>();
        const auto& component = registry.Get<Component>(inEntity);
        renderModule.GetRenderThread().EmplaceTask([scene = sceneHolder.scene.Get(), inEntity, component]() -> void {
            auto& sceneProxy = scene->Get<SceneProxy>(inEntity);
            Internal::UpdateSceneProxyContent(sceneProxy, component);
        });
    }

    template <typename SceneProxy>
    void SceneSystem::QueueUpdateSceneProxyTransform(Entity inEntity, bool inWithScale)
    {
        const auto& sceneHolder = registry.GGet<SceneHolder>();
        const auto& transform = registry.Get<WorldTransform>(inEntity);
        renderModule.GetRenderThread().EmplaceTask([scene = sceneHolder.scene.Get(), inEntity, transform, inWithScale]() -> void {
            auto& sceneProxy = scene->Get<SceneProxy>(inEntity);
            Internal::UpdateSceneProxyWorldTransform(sceneProxy, transform, inWithScale);
        });
    }

    template <typename SceneProxy>
    void SceneSystem::QueueRemoveSceneProxy(Entity inEntity)
    {
        const auto& sceneHolder = registry.GGet<SceneHolder>();
        renderModule.GetRenderThread().EmplaceTask([scene = sceneHolder.scene.Get(), inEntity]() -> void {
            scene->Remove<SceneProxy>(inEntity);
        });
    }
}

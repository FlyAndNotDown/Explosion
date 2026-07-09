//
// Created by johnk on 2026/7/4.
//

#include <Editor/EditorContext.h>
#include <Runtime/Component/Name.h>
#include <Runtime/Component/Transform.h>

namespace Editor {
    EditorContext::EditorContext()
        : sceneClient(Common::MakeUnique<SceneClient>())
        , selectedEntity(Runtime::entityNull)
        , selectionVersion(0)
        , worldStructureVersion(0)
        , componentsVersion(0)
    {
    }

    EditorContext::~EditorContext() = default;

    SceneClient& EditorContext::GetSceneClient() const
    {
        return *sceneClient;
    }

    Runtime::Entity EditorContext::GetSelectedEntity() const
    {
        return selectedEntity;
    }

    uint64_t EditorContext::GetSelectionVersion() const
    {
        return selectionVersion;
    }

    uint64_t EditorContext::GetWorldStructureVersion() const
    {
        return worldStructureVersion;
    }

    uint64_t EditorContext::GetComponentsVersion() const
    {
        return componentsVersion;
    }

    void EditorContext::SetSelectedEntity(Runtime::Entity inEntity)
    {
        if (selectedEntity == inEntity) {
            return;
        }
        selectedEntity = inEntity;
        selectionVersion++;
    }

    Runtime::Entity EditorContext::CreateEntity(const std::string& inName)
    {
        auto& registry = sceneClient->GetWorld().GetRegistry();
        const auto entity = registry.Create();
        // Name's reflected constructor takes a std::string.
        registry.Emplace<Runtime::Name>(entity, inName);
        registry.Emplace<Runtime::WorldTransform>(entity);
        worldStructureVersion++;
        return entity;
    }

    void EditorContext::DestroyEntity(Runtime::Entity inEntity)
    {
        auto& registry = sceneClient->GetWorld().GetRegistry();
        if (!registry.Valid(inEntity)) {
            return;
        }
        registry.Destroy(inEntity);
        if (selectedEntity == inEntity) {
            SetSelectedEntity(Runtime::entityNull);
        }
        worldStructureVersion++;
    }

    void EditorContext::RenameEntity(Runtime::Entity inEntity, const std::string& inName)
    {
        auto& registry = sceneClient->GetWorld().GetRegistry();
        if (!registry.Valid(inEntity)) {
            return;
        }
        if (registry.Has<Runtime::Name>(inEntity)) {
            registry.Update<Runtime::Name>(inEntity, [&](Runtime::Name& name) -> void { name.value = inName; });
        } else {
            registry.Emplace<Runtime::Name>(inEntity, inName);
        }
        worldStructureVersion++;
    }

    void EditorContext::NotifyComponentsChanged(Runtime::Entity)
    {
        componentsVersion++;
    }
}

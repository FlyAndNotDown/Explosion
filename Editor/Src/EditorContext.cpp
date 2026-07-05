//
// Created by johnk on 2026/7/4.
//

#include <Editor/EditorContext.h>
#include <Editor/moc_EditorContext.cpp> // NOLINT
#include <Runtime/Component/Name.h>
#include <Runtime/Component/Transform.h>

namespace Editor {
    EditorContext::EditorContext(QObject* inParent)
        : QObject(inParent)
        , client(Common::MakeUnique<EditorClient>())
        , selectedEntity(Runtime::entityNull)
    {
    }

    EditorContext::~EditorContext() = default;

    EditorClient& EditorContext::GetClient() const
    {
        return *client;
    }

    Runtime::Entity EditorContext::GetSelectedEntity() const
    {
        return selectedEntity;
    }

    void EditorContext::SetSelectedEntity(Runtime::Entity inEntity)
    {
        if (selectedEntity == inEntity) {
            return;
        }
        selectedEntity = inEntity;
        emit SelectionChanged(selectedEntity);
    }

    Runtime::Entity EditorContext::CreateEntity(const std::string& inName)
    {
        auto& registry = client->GetWorld().GetRegistry();
        const auto entity = registry.Create();
        // Name's reflected constructor takes a std::string
        registry.Emplace<Runtime::Name>(entity, inName);
        registry.Emplace<Runtime::WorldTransform>(entity);
        emit WorldStructureChanged();
        return entity;
    }

    void EditorContext::DestroyEntity(Runtime::Entity inEntity)
    {
        auto& registry = client->GetWorld().GetRegistry();
        if (!registry.Valid(inEntity)) {
            return;
        }
        registry.Destroy(inEntity);
        if (selectedEntity == inEntity) {
            SetSelectedEntity(Runtime::entityNull);
        }
        emit WorldStructureChanged();
    }

    void EditorContext::RenameEntity(Runtime::Entity inEntity, const std::string& inName)
    {
        auto& registry = client->GetWorld().GetRegistry();
        if (!registry.Valid(inEntity)) {
            return;
        }
        if (registry.Has<Runtime::Name>(inEntity)) {
            registry.Update<Runtime::Name>(inEntity, [&](Runtime::Name& name) -> void { name.value = inName; });
        } else {
            registry.Emplace<Runtime::Name>(inEntity, inName);
        }
        emit WorldStructureChanged();
    }

    void EditorContext::NotifyComponentsChanged(Runtime::Entity inEntity)
    {
        emit ComponentsChanged(inEntity);
    }
}

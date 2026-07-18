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

    bool EditorContext::CanAddComponent(const Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass) const
    {
        const bool isTag = inClass != nullptr && inClass->HasMeta(Runtime::MetaPresets::tag);
        if (!inRegistry.Valid(inEntity)
            || inClass == nullptr
            || !inClass->HasMeta("comp")
            || (!isTag && !inClass->HasDefaultConstructor())) {
            return false;
        }
        return isTag
            ? !inRegistry.HasTagDyn(inClass, inEntity)
            : !inRegistry.HasDyn(inClass, inEntity);
    }

    bool EditorContext::CanRemoveComponent(const Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass) const
    {
        if (!inRegistry.Valid(inEntity) || inClass == nullptr || !inClass->HasMeta("comp")) {
            return false;
        }
        return inClass->HasMeta(Runtime::MetaPresets::tag)
            ? inRegistry.HasTagDyn(inClass, inEntity)
            : inRegistry.HasDyn(inClass, inEntity);
    }

    void EditorContext::SetSelectedEntity(Runtime::Entity inEntity)
    {
        if (selectedEntity == inEntity) {
            return;
        }
        selectedEntity = inEntity;
    }

    Runtime::Entity EditorContext::CreateEntity(Runtime::ECRegistry& inRegistry, const std::string& inName)
    {
        const Runtime::Entity entity = inRegistry.Create();
        inRegistry.Emplace<Runtime::Name>(entity, inName);
        inRegistry.Emplace<Runtime::WorldTransform>(entity);
        return entity;
    }

    void EditorContext::DestroyEntity(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity)
    {
        if (!inRegistry.Valid(inEntity)) {
            return;
        }
        Runtime::HierarchyOps::Destroy(inRegistry, inEntity);
        if (selectedEntity == inEntity) {
            SetSelectedEntity(Runtime::entityNull);
        }
    }

    bool EditorContext::AddComponent(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass)
    {
        if (!CanAddComponent(inRegistry, inEntity, inClass)) {
            return false;
        }
        if (inClass->HasMeta(Runtime::MetaPresets::tag)) {
            inRegistry.AddTagDyn(inClass, inEntity);
        } else {
            inRegistry.EmplaceDyn(inClass, inEntity, {});
        }
        return true;
    }

    bool EditorContext::RemoveComponent(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass)
    {
        if (!CanRemoveComponent(inRegistry, inEntity, inClass)) {
            return false;
        }
        if (inClass->HasMeta(Runtime::MetaPresets::tag)) {
            inRegistry.RemoveTagDyn(inClass, inEntity);
        } else if (inClass == &Runtime::Hierarchy::GetStaticClass()) {
            Runtime::HierarchyOps::Remove(inRegistry, inEntity);
        } else {
            inRegistry.RemoveDyn(inClass, inEntity);
        }
        return true;
    }

    bool EditorContext::SetComponentMember(
        Runtime::ECRegistry& inRegistry,
        Runtime::Entity inEntity,
        Runtime::CompClass inClass,
        const Mirror::MemberVariable& inMemberVariable,
        const Mirror::Any& inValue)
    {
        if (inClass == nullptr
            || !inRegistry.Valid(inEntity)
            || !inRegistry.HasDyn(inClass, inEntity)
            || &inMemberVariable.GetOwner() != inClass) {
            return false;
        }
        inRegistry.UpdateDyn(inClass, inEntity, [&](const Mirror::Any& inComponent) -> void {
            inMemberVariable.SetDyn(inComponent, inValue);
        });
        return true;
    }
}

//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <Common/Memory.h>
#include <Editor/SceneClient.h>
#include <Mirror/Mirror.h>

namespace Editor {
    class EditorContext final {
    public:
        EditorContext();
        ~EditorContext();

        SceneClient& GetSceneClient() const;
        Runtime::Entity GetSelectedEntity() const;
        bool CanAddComponent(const Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass) const;
        bool CanRemoveComponent(const Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass) const;

        void SetSelectedEntity(Runtime::Entity inEntity);
        Runtime::Entity CreateEntity(Runtime::ECRegistry& inRegistry, const std::string& inName);
        void DestroyEntity(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity);
        bool AddComponent(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass);
        bool RemoveComponent(Runtime::ECRegistry& inRegistry, Runtime::Entity inEntity, Runtime::CompClass inClass);
        bool SetComponentMember(
            Runtime::ECRegistry& inRegistry,
            Runtime::Entity inEntity,
            Runtime::CompClass inClass,
            const Mirror::MemberVariable& inMemberVariable,
            const Mirror::Any& inValue);

    private:
        Common::UniquePtr<SceneClient> sceneClient;
        Runtime::Entity selectedEntity;
    };
}

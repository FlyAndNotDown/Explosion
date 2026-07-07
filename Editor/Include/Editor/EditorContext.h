//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <Common/Memory.h>
#include <Editor/SceneClient.h>

namespace Editor {
    class EditorContext final {
    public:
        EditorContext();
        ~EditorContext();

        SceneClient& GetSceneClient() const;
        Runtime::Entity GetSelectedEntity() const;
        uint64_t GetSelectionVersion() const;
        uint64_t GetWorldStructureVersion() const;
        uint64_t GetComponentsVersion() const;

        void SetSelectedEntity(Runtime::Entity inEntity);
        Runtime::Entity CreateEntity(const std::string& inName);
        void DestroyEntity(Runtime::Entity inEntity);
        void RenameEntity(Runtime::Entity inEntity, const std::string& inName);
        void NotifyComponentsChanged(Runtime::Entity inEntity);

    private:
        Common::UniquePtr<SceneClient> sceneClient;
        Runtime::Entity selectedEntity;
        uint64_t selectionVersion;
        uint64_t worldStructureVersion;
        uint64_t componentsVersion;
    };
}

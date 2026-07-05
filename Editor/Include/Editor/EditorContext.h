//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <QObject>

#include <Common/Memory.h>
#include <Editor/EditorClient.h>

namespace Editor {
    // single source of truth for one open editor session: owns the edited world's client, the selection state and
    // the change notifications shared across panels; all entity-level edits funnel through here so every panel
    // observes the same signals
    class EditorContext final : public QObject {
        Q_OBJECT

    public:
        explicit EditorContext(QObject* inParent = nullptr);
        ~EditorContext() override;

        EditorClient& GetClient() const;
        Runtime::Entity GetSelectedEntity() const;
        void SetSelectedEntity(Runtime::Entity inEntity);
        Runtime::Entity CreateEntity(const std::string& inName);
        void DestroyEntity(Runtime::Entity inEntity);
        void RenameEntity(Runtime::Entity inEntity, const std::string& inName);
        void NotifyComponentsChanged(Runtime::Entity inEntity);

    Q_SIGNALS:
        void SelectionChanged(Runtime::Entity inEntity);
        void WorldStructureChanged();
        void ComponentsChanged(Runtime::Entity inEntity);

    private:
        Common::UniquePtr<EditorClient> client;
        Runtime::Entity selectedEntity;
    };
}

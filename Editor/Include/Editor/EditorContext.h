//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <QObject>

#include <Common/Memory.h>
#include <Editor/EditorClient.h>

namespace Editor {
    // single source of truth for one open editor session: owns the edited world's client, later phases add the
    // selection state and change notifications shared across panels
    class EditorContext final : public QObject {
        Q_OBJECT

    public:
        explicit EditorContext(QObject* inParent = nullptr);
        ~EditorContext() override;

        EditorClient& GetClient() const;

    private:
        Common::UniquePtr<EditorClient> client;
    };
}

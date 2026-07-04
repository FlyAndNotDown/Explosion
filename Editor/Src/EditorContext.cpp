//
// Created by johnk on 2026/7/4.
//

#include <Editor/EditorContext.h>
#include <Editor/moc_EditorContext.cpp> // NOLINT

namespace Editor {
    EditorContext::EditorContext(QObject* inParent)
        : QObject(inParent)
        , client(Common::MakeUnique<EditorClient>())
    {
    }

    EditorContext::~EditorContext() = default;

    EditorClient& EditorContext::GetClient() const
    {
        return *client;
    }
}

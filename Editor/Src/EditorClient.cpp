//
// Created by johnk on 2026/7/4.
//

#include <Editor/EditorClient.h>
#include <Runtime/SystemGraphPresets.h>

namespace Editor {
    EditorClient::EditorClient()
        : world("EditorWorld", this, Runtime::PlayType::editor)
        , viewport(nullptr)
    {
        world.SetSystemGraph(Runtime::SystemGraphPresets::Default3DWorld());
        world.Activate();
    }

    EditorClient::~EditorClient()
    {
        if (world.Activated()) {
            world.Deactivate();
        }
    }

    Runtime::World& EditorClient::GetWorld()
    {
        return world;
    }

    Runtime::Viewport* EditorClient::GetViewport()
    {
        return viewport;
    }

    void EditorClient::SetViewport(Runtime::Viewport* inViewport)
    {
        viewport = inViewport;
    }
}

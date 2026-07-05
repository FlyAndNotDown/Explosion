//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <Core/Uri.h>
#include <Runtime/Client.h>
#include <Runtime/World.h>

namespace Editor {
    // connects the edited world with the editor viewport: owns the editor-play-type world and keeps it playing so
    // its systems (transform/scene/render) tick while editing, the viewport widget registers itself here on
    // construction
    class EditorClient final : public Runtime::Client {
    public:
        EditorClient();
        ~EditorClient() override;

        NonCopyable(EditorClient)
        NonMovable(EditorClient)

        Runtime::World& GetWorld() override;
        Runtime::Viewport* GetViewport() override;
        void SetViewport(Runtime::Viewport* inViewport);
        // loads the project's main level when present, otherwise authors the default level content (player start,
        // ground and a cube using engine-default unlit assets materialized into the project) and saves it
        void OpenProjectLevel();
        void SaveLevel();
        // the transient camera entity the editor viewport renders through and moves, entityNull before the level
        // is opened
        Runtime::Entity GetEditorCamera() const;

    private:
        void CreateEditorCamera();

        Core::Uri levelUri;
        Runtime::World world;
        Runtime::Viewport* viewport;
        Runtime::Entity editorCamera;
    };
}

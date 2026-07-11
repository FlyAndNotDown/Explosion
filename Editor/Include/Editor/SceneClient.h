//
// Created by johnk on 2026/7/4.
//

#pragma once

#include <cstdint>
#include <set>

#include <Common/Math/Vector.h>
#include <Core/Uri.h>
#include <Runtime/Client.h>
#include <Runtime/World.h>

namespace Editor {
    class EditorWindow;

    class SceneClient final : public Runtime::Client {
    public:
        SceneClient();
        ~SceneClient() override;

        NonCopyable(SceneClient)
        NonMovable(SceneClient)

        Runtime::World& GetWorld() override;
        Runtime::RenderSurface* GetRenderSurface() override;
        void SetEditorWindow(EditorWindow& inWindow);
        void SetRenderSurface(Runtime::RenderSurface* inRenderSurface);
        void ResizeRenderSurface(uint32_t inWidth, uint32_t inHeight);
        // loads the project's main level when present, otherwise authors the default level content (player start,
        // ground and a cube using engine-default unlit assets materialized into the project) and saves it
        void OpenProjectLevel();
        void SaveLevel();
        // the transient camera entity the editor scene renders through and moves, entityNull before the level is opened
        Runtime::Entity GetEditorCamera() const;
        void TickEditorCamera(float inDeltaSeconds);
        void SetSceneHovered(bool inHovered);
        void SetSceneFocused(bool inFocused);
        bool IsSceneHovered() const;
        bool IsCameraLooking() const;
        void OnKey(int inKey, bool inPressed);
        void BeginCameraLook();
        void EndCameraLook();
        void AddCameraLookDelta(float inDeltaX, float inDeltaY);

    private:
        void CreateEditorCamera();
        Common::FVec2 CameraMoveInput() const;

        Core::Uri levelUri;
        Runtime::World world;
        EditorWindow* window;
        Runtime::RenderSurface* renderSurface;
        Runtime::Entity editorCamera;
        std::set<int> pressedKeys;
        bool sceneHovered;
        bool cameraLooking;
        bool cameraAnglesInitialized;
        float cameraYaw;
        float cameraPitch;
        float pendingLookDeltaX;
        float pendingLookDeltaY;
    };
}

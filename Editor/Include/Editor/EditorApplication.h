//
// Created by johnk on 2026/7/7.
//

#pragma once

#include <Common/Memory.h>
#include <Editor/EditorContext.h>
#include <Editor/EditorWindow.h>
#include <Editor/Frame/EditorFrame.h>
#include <Editor/Frame/ProjectHubFrame.h>
#include <Runtime/Canvas.h>

namespace Editor {
    enum class EditorApplicationMode : uint8_t {
        projectHub,
        editor
    };

    struct EditorApplicationDesc {
        EditorApplicationMode mode;
        std::string rhiType;
        std::string projectRoot;
    };

    class EditorApplication final {
    public:
        explicit EditorApplication(EditorApplicationDesc inDesc);
        ~EditorApplication();

        NonCopyable(EditorApplication)
        NonMovable(EditorApplication)

        int Run();

    private:
        void InitializeImGui();
        void ShutdownImGui();
        void InstallCallbacks();
        void BeginImGuiFrame(float inDeltaSeconds) const;
        void DrawDockSpace() const;
        void RenderEditorFrame(float inDeltaSeconds);
        void RenderProjectHubFrame();
        void HandleKey(int inKey, int inScancode, int inAction, int inMods);
        void HandleChar(uint32_t inCodepoint) const;
        void HandleMouseButton(int inButton, int inAction, int inMods);
        void HandleCursor(double inX, double inY);
        void HandleScroll(double inXOffset, double inYOffset) const;
        float NextDeltaSeconds();

        EditorApplicationDesc desc;
        Common::UniquePtr<EditorWindow> window;
        Common::UniquePtr<EditorContext> context;
        Common::UniquePtr<Runtime::Canvas> sceneRenderCanvas;
        Common::UniquePtr<ProjectHubFrame> projectHubFrame;
        EditorFrame editorFrame;
        double lastFrameSeconds;
        double lastCursorX;
        double lastCursorY;
        bool hasLastCursor;
        bool requestQuit;
    };
}

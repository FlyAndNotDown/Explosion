//
// Created by johnk on 2026/7/7.
//

#pragma once

#include <string>
#include <vector>

#include <Editor/EditorContext.h>
#include <Editor/Panel/EditorPanels.h>
#include <Runtime/Canvas.h>

namespace Editor::Internal {
    struct EditorTabVisibility {
        bool scene;
        bool outliner;
        bool inspector;
        bool log;
    };
}

namespace Editor {
    class EditorFrame final {
    public:
        EditorFrame();
        ~EditorFrame();

        void Render(EditorContext& inContext, Runtime::ECRegistry& inRegistry, Runtime::Canvas& inSceneRenderCanvas, bool& outRequestQuit);

    private:
        void RenderMenuBar(EditorContext& inContext, bool& outRequestQuit);
        void RenderSceneTab(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas, bool& inOutOpen);
        void RenderOutlinerTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry, bool& inOutOpen);
        void RenderInspectorTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry, bool& inOutOpen);
        void RenderLogTab(bool& inOutOpen);

        std::string createEntityName;
        std::vector<Runtime::CompClass> componentClasses;
        Internal::EditorTabVisibility tabVisibility;
        EditorPanels panels;
    };
}

//
// Created by johnk on 2026/7/7.
//

#pragma once

#include <string>
#include <vector>

#include <Editor/EditorContext.h>
#include <Runtime/Canvas.h>

namespace Editor {
    class EditorFrame final {
    public:
        EditorFrame();
        ~EditorFrame();

        void Render(EditorContext& inContext, Runtime::ECRegistry& inRegistry, Runtime::Canvas& inSceneRenderCanvas, bool& outRequestQuit);

    private:
        void RenderMenuBar(EditorContext& inContext, bool& outRequestQuit);
        void RenderSceneTab(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas);
        void RenderOutlinerTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry);
        void RenderInspectorTab(EditorContext& inContext, Runtime::ECRegistry& inRegistry);
        void RenderLogTab();

        std::string createEntityName;
        int selectedAddComponentIndex;
        std::vector<Runtime::CompClass> componentClasses;
    };
}

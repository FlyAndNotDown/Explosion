//
// Created by johnk on 2026/7/7.
//

#pragma once

#include <string>

#include <Editor/EditorContext.h>
#include <Runtime/Canvas.h>

namespace Editor {
    class EditorFrame final {
    public:
        EditorFrame();
        ~EditorFrame();

        void Render(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas, bool& outRequestQuit);

    private:
        void RenderMenuBar(EditorContext& inContext, bool& outRequestQuit);
        void RenderSceneTab(EditorContext& inContext, Runtime::Canvas& inSceneRenderCanvas);
        void RenderOutlinerTab(EditorContext& inContext);
        void RenderInspectorTab(EditorContext& inContext);
        void RenderLogTab();

        std::string createEntityName;
        int selectedAddComponentIndex;
    };
}

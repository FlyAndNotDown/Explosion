//
// Created by johnk on 2026/7/18.
//

#pragma once

#include <optional>

#include <Editor/Panel/AssetsPanel.h>

namespace Editor {
    class EditorPanels final {
    public:
        EditorPanels();
        ~EditorPanels();

        void Render();
        void RenderViewMenuItems();

    private:
        std::optional<AssetsPanel> assetsPanel;
        bool assetsVisible;
    };
}

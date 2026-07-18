//
// Created by johnk on 2026/7/18.
//

#include <imgui.h>

#include <Editor/Panel/EditorPanelNames.h>
#include <Editor/Panel/EditorPanels.h>
#include <Editor/Widget/IconWidgets.h>
#include <Editor/Widget/TablerIcons.h>

namespace Editor {
    EditorPanels::EditorPanels()
        : assetsVisible(true)
    {
    }

    EditorPanels::~EditorPanels() = default;

    void EditorPanels::Render()
    {
        if (assetsVisible) {
            if (!assetsPanel) {
                assetsPanel.emplace();
            }
            assetsPanel->Render(assetsVisible);
        }
    }

    void EditorPanels::RenderViewMenuItems()
    {
        const std::string label = Widgets::Label(Icons::Tabler::folder, PanelNames::assets);
        ImGui::MenuItem(label.c_str(), nullptr, &assetsVisible);
    }
}

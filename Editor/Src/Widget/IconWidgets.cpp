#include <format>

#include <imgui.h>

#include <Editor/Widget/IconWidgets.h>

namespace Editor::Widgets {
    std::string Label(const char* inIcon, std::string_view inText)
    {
        return std::format("{} {}", inIcon, inText);
    }

    std::string Label(const char* inIcon, std::string_view inText, std::string_view inId)
    {
        return std::format("{} {}###{}", inIcon, inText, inId);
    }

    bool IconButton(const char* inId, const char* inIcon, const char* inTooltip)
    {
        ImGui::PushID(inId);
        const float size = ImGui::GetFrameHeight();
        const bool clicked = ImGui::Button(inIcon, ImVec2(size, size));
        if (inTooltip != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", inTooltip);
        }
        ImGui::PopID();
        return clicked;
    }
}

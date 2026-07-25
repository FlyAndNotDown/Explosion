#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include <Editor/Theme/Theme.h>
#include <Editor/Widget/IconWidgets.h>

namespace Editor::Widgets::Internal {
    static ImVec2 CalculateCenteredIconPosition(const char* inIcon, const ImVec2& inButtonMin, const ImVec2& inButtonMax)
    {
        unsigned int codepoint = 0;
        const int consumedBytes = ImTextCharFromUtf8(&codepoint, inIcon, nullptr);
        const ImFontGlyph* glyph = consumedBytes > 0 ? ImGui::GetFontBaked()->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)) : nullptr;
        if (glyph == nullptr) {
            const ImVec2 textSize = ImGui::CalcTextSize(inIcon);
            return ImVec2(inButtonMin.x + (inButtonMax.x - inButtonMin.x - textSize.x) * 0.5f, inButtonMin.y + (inButtonMax.y - inButtonMin.y - textSize.y) * 0.5f);
        }

        const float glyphWidth = glyph->X1 - glyph->X0;
        const float glyphHeight = glyph->Y1 - glyph->Y0;
        return ImVec2(inButtonMin.x + (inButtonMax.x - inButtonMin.x - glyphWidth) * 0.5f - glyph->X0, inButtonMin.y + (inButtonMax.y - inButtonMin.y - glyphHeight) * 0.5f - glyph->Y0);
    }
}

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
        const bool clicked = ImGui::Button("##IconButton", ImVec2(size, size));
        const ImVec2 iconPosition = Internal::CalculateCenteredIconPosition(inIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), iconPosition, ImGui::GetColorU32(ImGuiCol_Text), inIcon);
        if (inTooltip != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", inTooltip);
        }
        ImGui::PopID();
        return clicked;
    }

    bool PrimaryButton(const char* inLabel, const ImVec2& inSize)
    {
        const Theme& theme = ThemeSwitcher::GetCurrentTheme();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.GetOnAccentColor());
        ImGui::PushStyleColor(ImGuiCol_Button, theme.GetAccentColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme.GetAccentHoveredColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme.GetAccentActiveColor());
        const bool clicked = ImGui::Button(inLabel, inSize);
        ImGui::PopStyleColor(4);
        return clicked;
    }

    void PushWarningTextColor()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ThemeSwitcher::GetCurrentTheme().GetWarningColor());
    }

    void PushErrorTextColor()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ThemeSwitcher::GetCurrentTheme().GetErrorColor());
    }
}

#include <utility>

#include <imgui.h>

#include <Editor/Theme/Theme.h>

namespace Editor::Internal {
    static constexpr ImVec4 ThemeColorFromBytes(int inRed, int inGreen, int inBlue, int inAlpha = 255)
    {
        constexpr float scale = 1.0f / 255.0f;
        return {
            static_cast<float>(inRed) * scale,
            static_cast<float>(inGreen) * scale,
            static_cast<float>(inBlue) * scale,
            static_cast<float>(inAlpha) * scale,
        };
    }
}

namespace Editor {
    Theme::Theme()
        : textColor(Internal::ThemeColorFromBytes(235, 226, 213))
        , textDisabledColor(Internal::ThemeColorFromBytes(145, 132, 116))
        , windowBackgroundColor(Internal::ThemeColorFromBytes(17, 15, 13))
        , popupBackgroundColor(Internal::ThemeColorFromBytes(29, 25, 21))
        , surfaceColor(Internal::ThemeColorFromBytes(31, 27, 23))
        , surfaceHoveredColor(Internal::ThemeColorFromBytes(45, 36, 29))
        , surfaceActiveColor(Internal::ThemeColorFromBytes(65, 47, 35))
        , accentColor(Internal::ThemeColorFromBytes(250, 197, 118))
        , accentHoveredColor(Internal::ThemeColorFromBytes(255, 215, 151))
        , accentActiveColor(Internal::ThemeColorFromBytes(226, 168, 104))
        , onAccentColor(Internal::ThemeColorFromBytes(32, 22, 14))
        , warningColor(Internal::ThemeColorFromBytes(239, 177, 88))
        , errorColor(Internal::ThemeColorFromBytes(238, 104, 86))
    {
    }

    Theme::Theme(const Theme& inOther) = default;

    Theme::Theme(Theme&& inOther) noexcept = default;

    Theme::~Theme() = default;

    Theme& Theme::operator=(const Theme& inOther) = default;

    Theme& Theme::operator=(Theme&& inOther) noexcept = default;

    void Theme::Apply() const
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.DisabledAlpha = 0.55f;
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.WindowRounding = 6.0f;
        style.WindowBorderSize = 1.0f;
        style.ChildRounding = 5.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 8.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.FrameRounding = 5.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
        style.CellPadding = ImVec2(8.0f, 5.0f);
        style.IndentSpacing = 20.0f;
        style.ScrollbarSize = 13.0f;
        style.ScrollbarRounding = 7.0f;
        style.GrabMinSize = 10.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 5.0f;
        style.TabBorderSize = 0.0f;
        style.TabBarBorderSize = 1.0f;
        style.TabBarOverlineSize = 2.0f;
        style.SeparatorSize = 1.0f;
        style.DockingSeparatorSize = 2.0f;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = textColor;
        colors[ImGuiCol_TextDisabled] = textDisabledColor;
        colors[ImGuiCol_WindowBg] = windowBackgroundColor;
        colors[ImGuiCol_ChildBg] = Internal::ThemeColorFromBytes(20, 18, 16);
        colors[ImGuiCol_PopupBg] = popupBackgroundColor;
        colors[ImGuiCol_Border] = Internal::ThemeColorFromBytes(58, 48, 39);
        colors[ImGuiCol_BorderShadow] = Internal::ThemeColorFromBytes(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg] = Internal::ThemeColorFromBytes(34, 29, 25);
        colors[ImGuiCol_FrameBgHovered] = Internal::ThemeColorFromBytes(50, 40, 32);
        colors[ImGuiCol_FrameBgActive] = surfaceActiveColor;
        colors[ImGuiCol_TitleBg] = Internal::ThemeColorFromBytes(20, 17, 15);
        colors[ImGuiCol_TitleBgActive] = Internal::ThemeColorFromBytes(28, 23, 19);
        colors[ImGuiCol_TitleBgCollapsed] = Internal::ThemeColorFromBytes(20, 17, 15);
        colors[ImGuiCol_MenuBarBg] = Internal::ThemeColorFromBytes(23, 20, 17);
        colors[ImGuiCol_ScrollbarBg] = Internal::ThemeColorFromBytes(14, 13, 12);
        colors[ImGuiCol_ScrollbarGrab] = Internal::ThemeColorFromBytes(70, 56, 45);
        colors[ImGuiCol_ScrollbarGrabHovered] = Internal::ThemeColorFromBytes(102, 75, 54);
        colors[ImGuiCol_ScrollbarGrabActive] = Internal::ThemeColorFromBytes(150, 99, 63);
        colors[ImGuiCol_CheckMark] = accentColor;
        colors[ImGuiCol_CheckboxSelectedBg] = Internal::ThemeColorFromBytes(112, 72, 43);
        colors[ImGuiCol_SliderGrab] = Internal::ThemeColorFromBytes(195, 132, 74);
        colors[ImGuiCol_SliderGrabActive] = accentColor;
        colors[ImGuiCol_Button] = surfaceColor;
        colors[ImGuiCol_ButtonHovered] = surfaceHoveredColor;
        colors[ImGuiCol_ButtonActive] = surfaceActiveColor;
        colors[ImGuiCol_Header] = Internal::ThemeColorFromBytes(43, 35, 29);
        colors[ImGuiCol_HeaderHovered] = Internal::ThemeColorFromBytes(58, 43, 32);
        colors[ImGuiCol_HeaderActive] = Internal::ThemeColorFromBytes(78, 51, 34);
        colors[ImGuiCol_Separator] = Internal::ThemeColorFromBytes(55, 46, 38);
        colors[ImGuiCol_SeparatorHovered] = Internal::ThemeColorFromBytes(178, 116, 67);
        colors[ImGuiCol_SeparatorActive] = accentColor;
        colors[ImGuiCol_ResizeGrip] = Internal::ThemeColorFromBytes(200, 129, 87, 55);
        colors[ImGuiCol_ResizeGripHovered] = Internal::ThemeColorFromBytes(226, 168, 104, 170);
        colors[ImGuiCol_ResizeGripActive] = accentColor;
        colors[ImGuiCol_InputTextCursor] = accentColor;
        colors[ImGuiCol_TabHovered] = Internal::ThemeColorFromBytes(55, 41, 31);
        colors[ImGuiCol_Tab] = Internal::ThemeColorFromBytes(25, 22, 19);
        colors[ImGuiCol_TabSelected] = Internal::ThemeColorFromBytes(40, 32, 26);
        colors[ImGuiCol_TabSelectedOverline] = accentColor;
        colors[ImGuiCol_TabDimmed] = Internal::ThemeColorFromBytes(20, 18, 16);
        colors[ImGuiCol_TabDimmedSelected] = Internal::ThemeColorFromBytes(31, 27, 23);
        colors[ImGuiCol_TabDimmedSelectedOverline] = Internal::ThemeColorFromBytes(200, 129, 87);
        colors[ImGuiCol_DockingPreview] = Internal::ThemeColorFromBytes(250, 197, 118, 150);
        colors[ImGuiCol_DockingEmptyBg] = Internal::ThemeColorFromBytes(12, 11, 10);
        colors[ImGuiCol_PlotLines] = Internal::ThemeColorFromBytes(211, 181, 139);
        colors[ImGuiCol_PlotLinesHovered] = accentColor;
        colors[ImGuiCol_PlotHistogram] = Internal::ThemeColorFromBytes(200, 129, 87);
        colors[ImGuiCol_PlotHistogramHovered] = Internal::ThemeColorFromBytes(239, 165, 105);
        colors[ImGuiCol_TableHeaderBg] = Internal::ThemeColorFromBytes(37, 31, 26);
        colors[ImGuiCol_TableBorderStrong] = Internal::ThemeColorFromBytes(66, 53, 43);
        colors[ImGuiCol_TableBorderLight] = Internal::ThemeColorFromBytes(48, 40, 33);
        colors[ImGuiCol_TableRowBg] = Internal::ThemeColorFromBytes(0, 0, 0, 0);
        colors[ImGuiCol_TableRowBgAlt] = Internal::ThemeColorFromBytes(255, 224, 185, 8);
        colors[ImGuiCol_TextLink] = Internal::ThemeColorFromBytes(239, 182, 111);
        colors[ImGuiCol_TextSelectedBg] = Internal::ThemeColorFromBytes(250, 197, 118, 70);
        colors[ImGuiCol_TreeLines] = Internal::ThemeColorFromBytes(72, 59, 48);
        colors[ImGuiCol_DragDropTarget] = accentColor;
        colors[ImGuiCol_DragDropTargetBg] = Internal::ThemeColorFromBytes(250, 197, 118, 25);
        colors[ImGuiCol_UnsavedMarker] = Internal::ThemeColorFromBytes(200, 129, 87);
        colors[ImGuiCol_NavCursor] = accentColor;
        colors[ImGuiCol_NavWindowingHighlight] = Internal::ThemeColorFromBytes(250, 222, 180, 180);
        colors[ImGuiCol_NavWindowingDimBg] = Internal::ThemeColorFromBytes(8, 7, 6, 145);
        colors[ImGuiCol_ModalWindowDimBg] = Internal::ThemeColorFromBytes(8, 7, 6, 180);
    }

    Theme& Theme::SetTextColor(const ImVec4& inColor)
    {
        textColor = inColor;
        return *this;
    }

    Theme& Theme::SetTextDisabledColor(const ImVec4& inColor)
    {
        textDisabledColor = inColor;
        return *this;
    }

    Theme& Theme::SetWindowBackgroundColor(const ImVec4& inColor)
    {
        windowBackgroundColor = inColor;
        return *this;
    }

    Theme& Theme::SetPopupBackgroundColor(const ImVec4& inColor)
    {
        popupBackgroundColor = inColor;
        return *this;
    }

    Theme& Theme::SetSurfaceColor(const ImVec4& inColor)
    {
        surfaceColor = inColor;
        return *this;
    }

    Theme& Theme::SetSurfaceHoveredColor(const ImVec4& inColor)
    {
        surfaceHoveredColor = inColor;
        return *this;
    }

    Theme& Theme::SetSurfaceActiveColor(const ImVec4& inColor)
    {
        surfaceActiveColor = inColor;
        return *this;
    }

    Theme& Theme::SetAccentColor(const ImVec4& inColor)
    {
        accentColor = inColor;
        return *this;
    }

    Theme& Theme::SetAccentHoveredColor(const ImVec4& inColor)
    {
        accentHoveredColor = inColor;
        return *this;
    }

    Theme& Theme::SetAccentActiveColor(const ImVec4& inColor)
    {
        accentActiveColor = inColor;
        return *this;
    }

    Theme& Theme::SetOnAccentColor(const ImVec4& inColor)
    {
        onAccentColor = inColor;
        return *this;
    }

    Theme& Theme::SetWarningColor(const ImVec4& inColor)
    {
        warningColor = inColor;
        return *this;
    }

    Theme& Theme::SetErrorColor(const ImVec4& inColor)
    {
        errorColor = inColor;
        return *this;
    }

    const ImVec4& Theme::GetTextColor() const
    {
        return textColor;
    }

    const ImVec4& Theme::GetTextDisabledColor() const
    {
        return textDisabledColor;
    }

    const ImVec4& Theme::GetWindowBackgroundColor() const
    {
        return windowBackgroundColor;
    }

    const ImVec4& Theme::GetPopupBackgroundColor() const
    {
        return popupBackgroundColor;
    }

    const ImVec4& Theme::GetSurfaceColor() const
    {
        return surfaceColor;
    }

    const ImVec4& Theme::GetSurfaceHoveredColor() const
    {
        return surfaceHoveredColor;
    }

    const ImVec4& Theme::GetSurfaceActiveColor() const
    {
        return surfaceActiveColor;
    }

    const ImVec4& Theme::GetAccentColor() const
    {
        return accentColor;
    }

    const ImVec4& Theme::GetAccentHoveredColor() const
    {
        return accentHoveredColor;
    }

    const ImVec4& Theme::GetAccentActiveColor() const
    {
        return accentActiveColor;
    }

    const ImVec4& Theme::GetOnAccentColor() const
    {
        return onAccentColor;
    }

    const ImVec4& Theme::GetWarningColor() const
    {
        return warningColor;
    }

    const ImVec4& Theme::GetErrorColor() const
    {
        return errorColor;
    }

    Theme ThemeSwitcher::currentTheme;

    const Theme& ThemeSwitcher::GetCurrentTheme()
    {
        return currentTheme;
    }

    void ThemeSwitcher::SwitchTheme(Theme inTheme)
    {
        currentTheme = std::move(inTheme);
        if (ImGui::GetCurrentContext() != nullptr) {
            ApplyCurrentTheme();
        }
    }

    void ThemeSwitcher::ApplyCurrentTheme()
    {
        currentTheme.Apply();
    }
}

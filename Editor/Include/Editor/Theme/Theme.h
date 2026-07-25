#pragma once

#include <imgui.h>

namespace Editor {
    class Theme final {
    public:
        Theme();
        Theme(const Theme& inOther);
        Theme(Theme&& inOther) noexcept;
        ~Theme();

        Theme& operator=(const Theme& inOther);
        Theme& operator=(Theme&& inOther) noexcept;

        void Apply() const;

        Theme& SetTextColor(const ImVec4& inColor);
        Theme& SetTextDisabledColor(const ImVec4& inColor);
        Theme& SetWindowBackgroundColor(const ImVec4& inColor);
        Theme& SetPopupBackgroundColor(const ImVec4& inColor);
        Theme& SetSurfaceColor(const ImVec4& inColor);
        Theme& SetSurfaceHoveredColor(const ImVec4& inColor);
        Theme& SetSurfaceActiveColor(const ImVec4& inColor);
        Theme& SetAccentColor(const ImVec4& inColor);
        Theme& SetAccentHoveredColor(const ImVec4& inColor);
        Theme& SetAccentActiveColor(const ImVec4& inColor);
        Theme& SetOnAccentColor(const ImVec4& inColor);
        Theme& SetWarningColor(const ImVec4& inColor);
        Theme& SetErrorColor(const ImVec4& inColor);

        const ImVec4& GetTextColor() const;
        const ImVec4& GetTextDisabledColor() const;
        const ImVec4& GetWindowBackgroundColor() const;
        const ImVec4& GetPopupBackgroundColor() const;
        const ImVec4& GetSurfaceColor() const;
        const ImVec4& GetSurfaceHoveredColor() const;
        const ImVec4& GetSurfaceActiveColor() const;
        const ImVec4& GetAccentColor() const;
        const ImVec4& GetAccentHoveredColor() const;
        const ImVec4& GetAccentActiveColor() const;
        const ImVec4& GetOnAccentColor() const;
        const ImVec4& GetWarningColor() const;
        const ImVec4& GetErrorColor() const;

    private:
        ImVec4 textColor;
        ImVec4 textDisabledColor;
        ImVec4 windowBackgroundColor;
        ImVec4 popupBackgroundColor;
        ImVec4 surfaceColor;
        ImVec4 surfaceHoveredColor;
        ImVec4 surfaceActiveColor;
        ImVec4 accentColor;
        ImVec4 accentHoveredColor;
        ImVec4 accentActiveColor;
        ImVec4 onAccentColor;
        ImVec4 warningColor;
        ImVec4 errorColor;
    };

    class ThemeSwitcher final {
    public:
        ThemeSwitcher() = delete;
        ~ThemeSwitcher() = delete;

        static const Theme& GetCurrentTheme();
        static void SwitchTheme(Theme inTheme);
        static void ApplyCurrentTheme();

    private:
        static Theme currentTheme;
    };
}

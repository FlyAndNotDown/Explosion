#pragma once

#include <string>
#include <string_view>

struct ImVec2;

namespace Editor::Widgets {
    std::string Label(const char* inIcon, std::string_view inText);
    std::string Label(const char* inIcon, std::string_view inText, std::string_view inId);
    bool IconButton(const char* inId, const char* inIcon, const char* inTooltip);
    bool PrimaryButton(const char* inLabel, const ImVec2& inSize);
    void PushWarningTextColor();
    void PushErrorTextColor();
}

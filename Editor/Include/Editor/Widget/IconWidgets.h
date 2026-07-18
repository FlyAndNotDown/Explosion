#pragma once

#include <string>
#include <string_view>

namespace Editor::Widgets {
    std::string Label(const char* inIcon, std::string_view inText);
    std::string Label(const char* inIcon, std::string_view inText, std::string_view inId);
    bool IconButton(const char* inId, const char* inIcon, const char* inTooltip);
}

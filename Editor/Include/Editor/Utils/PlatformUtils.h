//
// Created by johnk on 2026/7/11.
//

#pragma once

#include <optional>
#include <string>

namespace Editor {
    class PlatformUtils final {
    public:
        static std::optional<std::string> SelectDirectory(const std::string& inTitle, const std::string& inInitialDirectory = {});
    };
}

#pragma once

#include <optional>

#include <imgui.h>

namespace Editor {
    class ImGuiCompatibility final {
    public:
        static ImGuiKey ToImGuiKey(int inGlfwKey);
        static std::optional<ImGuiMouseButton> ToImGuiMouseButton(int inGlfwButton);
        static void UpdateKeyModifiers(int inGlfwMods);
    };
}

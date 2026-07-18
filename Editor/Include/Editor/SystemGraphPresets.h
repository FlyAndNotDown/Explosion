#pragma once

#include <Runtime/ECS.h>

namespace Editor {
    class EditorSystemGraphPresets final {
    public:
        static const Runtime::SystemGraph& DefaultEditorWorld();

        EditorSystemGraphPresets() = delete;
    };
}

//
// Created by johnk on 2024/8/21.
//

#pragma once

#include <Runtime/Engine.h>

namespace Editor {
    class EditorModule final : public Runtime::EngineModule {
    public:
        void OnUnload() override;
        ::Core::ModuleType Type() const override;
        Runtime::Engine* CreateEngine(const Runtime::EngineInitParams&) override;
    };
}

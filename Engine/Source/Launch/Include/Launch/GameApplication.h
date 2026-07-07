//
// Created by johnk on 2025/3/24.
//

#pragma once

#include <Common/Memory.h>
#include <Launch/GameClient.h>
#include <Launch/GameWindow.h>
#include <Runtime/Engine.h>
#include <Runtime/GameModule.h>

namespace Launch {
    class GameApplication {
    public:
        GameApplication(int argc, char* argv[]);
        ~GameApplication();

        void Tick();
        bool ShouldClose() const;

    private:
        double lastFrameTimeSeconds;
        double thisFrameTimeSeconds;
        float deltaTimeSeconds;
        Common::UniquePtr<GameWindow> window;
        Common::UniquePtr<GameClient> client;
        Runtime::Engine* engine;
        Runtime::GameModule* gameModule;
    };
}

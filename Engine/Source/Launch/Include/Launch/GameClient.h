//
// Created by johnk on 2025/2/19.
//

#pragma once

#include <Runtime/Client.h>
#include <Runtime/World.h>

namespace Launch {
    class GameWindow;

    class GameClient final : public Runtime::Client {
    public:
        explicit GameClient(GameWindow& inWindow);
        ~GameClient() override;

        Runtime::World& GetWorld() override;
        Runtime::RenderSurface* GetRenderSurface() override;

    private:
        GameWindow& window;
        Runtime::World world;
    };
}

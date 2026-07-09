//
// Created by johnk on 2025/2/19.
//

#include <Launch/GameClient.h>
#include <Launch/GameWindow.h>
#include <Runtime/SystemGraphPresets.h>

namespace Launch {
    GameClient::GameClient(GameWindow& inWindow)
        : window(inWindow)
        , world("GameWorld", this, Runtime::PlayType::game)
    {
        world.SetSystemGraph(Runtime::SystemGraphPresets::Default3DWorld());
    }

    GameClient::~GameClient() = default;

    Runtime::World& GameClient::GetWorld()
    {
        return world;
    }

    Runtime::RenderSurface* GameClient::GetRenderSurface()
    {
        return &window;
    }
}

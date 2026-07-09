//
// Created by johnk on 2025/3/13.
//

#pragma once

#include <Runtime/Meta.h>
#include <Runtime/ECS.h>
#include <Runtime/Api.h>

namespace Runtime {
    struct RUNTIME_API EClass(globalComp, transient) PlayersInfo {
        EClassBody(PlayersInfo)

        PlayersInfo();

        std::vector<Entity> players;
    };

    struct RUNTIME_API EClass(transient) LocalPlayer {
        EClassBody(Player)

        LocalPlayer();

        uint8_t localPlayerIndex;
    };

    struct RUNTIME_API EClass(comp) PlayerStart {
        EClassBody(PlayerStart)

        PlayerStart();
    };
}

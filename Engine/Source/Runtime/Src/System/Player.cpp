//
// Created by johnk on 2025/3/13.
//

#include <Common/Debug.h>
#include <Runtime/Component/Camera.h>
#include <Runtime/Component/Player.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/Settings/Game.h>
#include <Runtime/Settings/Registry.h>
#include <Runtime/System/Player.h>

namespace Runtime {
    PlayerSystem::PlayerSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext)
        : System(inRegistry, inContext)
        , activeLocalPlayerNum(0)
    {
        // an editor world renders through the editor camera instead of players, so only game worlds spawn them
        auto& playersInfo = registry.GEmplace<PlayersInfo>();
        if (inContext.playType != PlayType::game) {
            return;
        }

        const auto& gameSettings = SettingsRegistry::Get().GetSettings<GameSettings>();
        playersInfo.players.resize(gameSettings.initialLocalPlayerNum);
        for (auto& playerEntity : playersInfo.players) {
            playerEntity = CreateLocalPlayer();
        }
    }

    PlayerSystem::~PlayerSystem() = default;

    Entity PlayerSystem::CreateLocalPlayer()
    {
        const auto playerStarts = registry.View<PlayerStart, WorldTransform>().All();
        Assert(!playerStarts.empty());

        const auto playerEntity = registry.Create();
        registry.AddTag<TransientTag>(playerEntity);
        registry.Emplace<Camera>(playerEntity);
        registry.Emplace<WorldTransform>(playerEntity, std::get<2>(playerStarts.front()).localToWorld);
        registry.Emplace<LocalPlayer>(playerEntity).localPlayerIndex = activeLocalPlayerNum++;
        return playerEntity;
    }
} // namespace Runtime

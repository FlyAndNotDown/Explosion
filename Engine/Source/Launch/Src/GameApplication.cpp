//
// Created by johnk on 2025/3/24.
//

#include <Common/Time.h>
#include <Launch/GameApplication.h>
#include <Core/Cmdline.h>
#include <RHI/Instance.h>
#include <Runtime/Engine.h>
#include <Runtime/Settings/Registry.h>
#include <Runtime/Settings/Game.h>

namespace Launch {
    extern std::string gameModuleName;

    static Core::CmdlineArgValue<std::string> caRhiType(
        "rhiType", "-rhi", RHI::GetPlatformDefaultRHIAbbrString(),
        "rhi abbr string, can be 'dx12' or 'vulkan'");

    GameApplication::GameApplication(int argc, char* argv[])
        : lastFrameTimeSeconds(Common::TimePoint::Now().ToSeconds())
        , thisFrameTimeSeconds(Common::TimePoint::Now().ToSeconds())
        , deltaTimeSeconds(0)
    {
        Core::Cli::Get().Parse(argc, argv);

        Runtime::EngineInitParams engineInitParams;
        engineInitParams.logToFile = true;
        engineInitParams.rhiType = caRhiType.GetValue();
        Runtime::EngineHolder::Load(gameModuleName, engineInitParams);

        engine = &Runtime::EngineHolder::Get();
        gameModule = &Core::ModuleManager::Get().GetTyped<Runtime::GameModule>(gameModuleName);

        GameWindowDesc windowDesc;
        windowDesc.title = gameModule->GetGameName();
        windowDesc.width = 1024;
        windowDesc.height = 768;
        window = Common::MakeUnique<GameWindow>(windowDesc);
        client = Common::MakeUnique<GameClient>(*window);

        auto& settingRegistry = Runtime::SettingsRegistry::Get();
        settingRegistry.LoadAllSettings();

        const auto& gameSettings = Runtime::SettingsRegistry::Get().GetSettings<Runtime::GameSettings>();
        const auto startupLevel = Runtime::AssetManager::Get().SyncLoad<Runtime::Level>(gameSettings.gameStartupLevel, Runtime::Level::GetStaticClass());
        client->GetWorld().LoadFrom(startupLevel);
        client->GetWorld().Play();
    }

    GameApplication::~GameApplication()
    {
        if (client.Valid() && !client->GetWorld().Stopped()) {
            client->GetWorld().Stop();
        }
        client.Reset();
        window.Reset();
        Runtime::EngineHolder::Unload();
    }

    void GameApplication::Tick()
    {
        thisFrameTimeSeconds = Common::TimePoint::Now().ToSeconds();
        deltaTimeSeconds = thisFrameTimeSeconds - lastFrameTimeSeconds;
        lastFrameTimeSeconds = thisFrameTimeSeconds;

        engine->Tick(deltaTimeSeconds);
        window->PollEvents();
    }

    bool GameApplication::ShouldClose() const
    {
        return window->ShouldClose();
    }
}

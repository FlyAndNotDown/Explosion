//
// Created by johnk on 2024/10/31.
//

#include <Runtime/World.h>
#include <Runtime/Engine.h>

#include <utility>

namespace Runtime {
    World::World(std::string inName, Client* inClient, PlayType inPlayType)
        : name(std::move(inName))
        , playStatus(PlayStatus::stopped)
        , systemSetupContext()
    {
        EngineHolder::Get().MountWorld(this);

        systemSetupContext.playType = inPlayType;
        systemSetupContext.client = inClient;
    }

    World::~World()
    {
        EngineHolder::Get().UnmountWorld(this);
    }

    void World::SetSystemGraph(const SystemGraph& inSystemGraph)
    {
        systemGraph = inSystemGraph;
    }

    Runtime::PlayStatus World::PlayStatus() const
    {
        return playStatus;
    }

    bool World::Stopped() const
    {
        return playStatus == PlayStatus::stopped;
    }

    bool World::Playing() const
    {
        return playStatus == PlayStatus::playing;
    }

    bool World::Paused() const
    {
        return playStatus == PlayStatus::paused;
    }

    void World::Play()
    {
        Assert(Stopped() && !executor.has_value());
        playStatus = PlayStatus::playing;
        executor.emplace(ecRegistry, systemGraph, systemSetupContext);
    }

    void World::Resume()
    {
        Assert(Paused());
        playStatus = PlayStatus::playing;
    }

    void World::Pause()
    {
        Assert(Playing());
        playStatus = PlayStatus::paused;
    }

    void World::Stop()
    {
        Assert((Playing() || Paused()) && executor.has_value());
        playStatus = PlayStatus::stopped;
        executor.reset();
    }

    void World::Activate()
    {
        Assert(systemSetupContext.playType == PlayType::editor && Stopped() && !executor.has_value());
        executor.emplace(ecRegistry, systemGraph, systemSetupContext);
    }

    void World::Deactivate()
    {
        Assert(systemSetupContext.playType == PlayType::editor && Stopped() && executor.has_value());
        executor.reset();
    }

    bool World::Activated() const
    {
        return executor.has_value();
    }

    bool World::ShouldTick() const
    {
        return executor.has_value() && !Paused();
    }

    ECRegistry& World::GetRegistry()
    {
        return ecRegistry;
    }

    const ECRegistry& World::GetRegistry() const
    {
        return ecRegistry;
    }

    void World::LoadFrom(AssetPtr<Level> inLevel)
    {
        Assert(Stopped());
        ecRegistry.Load(inLevel->GetArchive());
    }

    void World::SaveTo(AssetPtr<Level> inLevel)
    {
        Assert(Stopped());
        ecRegistry.Save(inLevel->GetArchive());
    }

    void World::Tick(float inDeltaTimeSeconds)
    {
        executor->Tick(inDeltaTimeSeconds);
    }
} // namespace Runtime

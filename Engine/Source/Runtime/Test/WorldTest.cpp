//
// Created by johnk on 2024/8/20.
//

#include <WorldTest.h>
#include <Runtime/Engine.h>
#include <Test/Test.h>

struct WorldTest : testing::Test {
    void SetUp() override
    {
        EngineInitParams initParams;
        initParams.projectFile = "";
        initParams.rhiType = RHI::GetPlatformDefaultRHIAbbrString();
        EngineHolder::Load("RuntimeTest", initParams);
    }
};

GlobalCounter::GlobalCounter()
    : tickTime(0)
    , value(0)
{
}

ParralCountSystemA::ParralCountSystemA() = default;

ParralCountSystemA::~ParralCountSystemA() = default;

void ParralCountSystemA::Tick(Commands& commands, float inTimeMs) const
{
    commands.PatchState<GlobalCounter>([](auto& state) -> void {
        state.tickTime += 1;
        state.value += 2;
    });
}

ParralCountSystemB::ParralCountSystemB() = default;

ParralCountSystemB::~ParralCountSystemB() = default;

void ParralCountSystemB::Tick(Commands& commands, float inTimeMs) const
{
    commands.PatchState<GlobalCounter>([](auto& state) -> void {
        state.value += 3;
    });
}

GlobalCounterVerifySystem::GlobalCounterVerifySystem() = default;

GlobalCounterVerifySystem::~GlobalCounterVerifySystem() = default;

void GlobalCounterVerifySystem::Setup(Commands& commands) const
{
    commands.EmplaceState<GlobalCounter>();
}

void GlobalCounterVerifySystem::Tick(Commands& commands, float inTimeMs) const
{
    const auto& globalCounter = commands.GetState<GlobalCounter>();
    ASSERT_EQ(globalCounter.value, globalCounter.tickTime * 5);
}

TEST_F(WorldTest, ECSBasic)
{
    World world;
    world.AddSystem<ParralCountSystemA>();
    world.AddSystem<ParralCountSystemB>();
    world.AddBarrier();
    world.AddSystem<GlobalCounterVerifySystem>();
    world.AddBarrier();

    world.Play();
    world.Tick(3.0f);
    world.Tick(3.0f);
    world.Stop();
}

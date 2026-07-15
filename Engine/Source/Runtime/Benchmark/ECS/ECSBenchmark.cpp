//
// Created by johnk on 2026/7/12.
//

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>
#include <entt/entt.hpp>
#include <flecs.h>

#include <ECSBenchmark.h>
#include <Runtime/ECS.h>

namespace Runtime::ECSBenchmark {
    Position::Position()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

    Position::Position(float inX, float inY, float inZ)
        : x(inX)
        , y(inY)
        , z(inZ)
    {
    }

    Velocity::Velocity()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

    Velocity::Velocity(float inX, float inY, float inZ)
        : x(inX)
        , y(inY)
        , z(inZ)
    {
    }

    Health::Health()
        : current(0)
        , maximum(0)
    {
    }

    Health::Health(uint32_t inCurrent, uint32_t inMaximum)
        : current(inCurrent)
        , maximum(inMaximum)
    {
    }

    Payload::Payload()
        : values {}
    {
    }

    Payload::Payload(uint64_t inSeed)
    {
        for (size_t i = 0; i < values.size(); i++) {
            values[i] = inSeed + i;
        }
    }
}

namespace Runtime::ECSBenchmark::Internal {
    constexpr int64_t smallEntityCount = 1 << 10;
    constexpr int64_t mediumEntityCount = 1 << 13;
    constexpr int64_t largeEntityCount = 1 << 16;

    struct ExplosionBackend {
        static constexpr std::string_view name = "Explosion";

        using Registry = ECRegistry;
        using Entity = Runtime::Entity;
        using MotionView = decltype(std::declval<Registry&>().View<Position, Velocity>());
        using FilteredMotionView = decltype(std::declval<Registry&>().View<Position, Velocity>(Exclude<Health> {}));
        using DynamicMotionView = RuntimeView;

        static Entity Create(Registry& inRegistry)
        {
            return inRegistry.Create();
        }

        static void Destroy(Registry& inRegistry, Entity inEntity)
        {
            inRegistry.Destroy(inEntity);
        }

        template <typename C, typename... Args>
        static void Emplace(Registry& inRegistry, Entity inEntity, Args&&... inArgs)
        {
            inRegistry.Emplace<C>(inEntity, std::forward<Args>(inArgs)...);
        }

        template <typename C>
        static void Remove(Registry& inRegistry, Entity inEntity)
        {
            inRegistry.Remove<C>(inEntity);
        }

        template <typename C>
        static const C& Get(const Registry& inRegistry, Entity inEntity)
        {
            return inRegistry.Get<C>(inEntity);
        }

        static MotionView MakeMotionView(Registry& inRegistry)
        {
            return inRegistry.View<Position, Velocity>();
        }

        static FilteredMotionView MakeFilteredMotionView(Registry& inRegistry)
        {
            return inRegistry.View<Position, Velocity>(Exclude<Health> {});
        }

        static DynamicMotionView MakeDynamicMotionView(Registry& inRegistry)
        {
            return inRegistry.RuntimeView(RuntimeFilter().Include<Position>().Include<Velocity>());
        }

        template <typename F>
        static void EachMotion(MotionView& inView, F&& inFunc)
        {
            inView.Each([&](Entity, const Position& position, const Velocity& velocity) -> void {
                inFunc(position, velocity);
            });
        }

        template <typename F>
        static void EachDynamicMotion(Registry&, DynamicMotionView& inView, F&& inFunc)
        {
            inView.Each([&](Entity, const Position& position, const Velocity& velocity) -> void {
                inFunc(position, velocity);
            });
        }
    };

    struct EnTTBackend {
        static constexpr std::string_view name = "EnTT";

        using Registry = entt::registry;
        using Entity = entt::entity;
        using MotionView = decltype(std::declval<Registry&>().view<Position, Velocity>());
        using FilteredMotionView = decltype(std::declval<Registry&>().view<Position, Velocity>(entt::exclude<Health>));
        using DynamicMotionView = entt::runtime_view;

        static Entity Create(Registry& inRegistry)
        {
            return inRegistry.create();
        }

        static void Destroy(Registry& inRegistry, Entity inEntity)
        {
            inRegistry.destroy(inEntity);
        }

        template <typename C, typename... Args>
        static void Emplace(Registry& inRegistry, Entity inEntity, Args&&... inArgs)
        {
            inRegistry.emplace<C>(inEntity, std::forward<Args>(inArgs)...);
        }

        template <typename C>
        static void Remove(Registry& inRegistry, Entity inEntity)
        {
            inRegistry.remove<C>(inEntity);
        }

        template <typename C>
        static const C& Get(const Registry& inRegistry, Entity inEntity)
        {
            return inRegistry.get<C>(inEntity);
        }

        static MotionView MakeMotionView(Registry& inRegistry)
        {
            return inRegistry.view<Position, Velocity>();
        }

        static FilteredMotionView MakeFilteredMotionView(Registry& inRegistry)
        {
            return inRegistry.view<Position, Velocity>(entt::exclude<Health>);
        }

        static DynamicMotionView MakeDynamicMotionView(Registry& inRegistry)
        {
            DynamicMotionView result;
            result.iterate(inRegistry.storage<Position>());
            result.iterate(inRegistry.storage<Velocity>());
            return result;
        }

        template <typename F>
        static void EachMotion(MotionView& inView, F&& inFunc)
        {
            inView.each([&](Entity, const Position& position, const Velocity& velocity) -> void {
                inFunc(position, velocity);
            });
        }

        template <typename F>
        static void EachDynamicMotion(Registry& inRegistry, DynamicMotionView& inView, F&& inFunc)
        {
            for (const auto entity : inView) {
                inFunc(inRegistry.get<Position>(entity), inRegistry.get<Velocity>(entity));
            }
        }
    };

    struct FlecsBackend {
        static constexpr std::string_view name = "flecs";

        using Registry = flecs::world;
        using Entity = flecs::entity;
        using MotionView = flecs::query<const Position, const Velocity>;
        using FilteredMotionView = flecs::query<const Position, const Velocity>;
        using DynamicMotionView = flecs::query<>;

        static Entity Create(Registry& inRegistry)
        {
            return inRegistry.entity();
        }

        static void Destroy(Registry&, Entity inEntity)
        {
            inEntity.destruct();
        }

        template <typename C, typename... Args>
        static void Emplace(Registry&, Entity inEntity, Args&&... inArgs)
        {
            inEntity.emplace<C>(std::forward<Args>(inArgs)...);
        }

        template <typename C>
        static void Remove(Registry&, Entity inEntity)
        {
            inEntity.remove<C>();
        }

        template <typename C>
        static const C& Get(const Registry&, Entity inEntity)
        {
            return inEntity.get<C>();
        }

        static MotionView MakeMotionView(Registry& inRegistry)
        {
            return inRegistry.query<const Position, const Velocity>();
        }

        static FilteredMotionView MakeFilteredMotionView(Registry& inRegistry)
        {
            return inRegistry.query_builder<const Position, const Velocity>().without<Health>().build();
        }

        static DynamicMotionView MakeDynamicMotionView(Registry& inRegistry)
        {
            return inRegistry.query_builder<>()
                .with<const Position>()
                .with<const Velocity>()
                .build();
        }

        template <typename F>
        static void EachMotion(MotionView& inView, F&& inFunc)
        {
            inView.each([&](const Position& position, const Velocity& velocity) -> void {
                inFunc(position, velocity);
            });
        }

        template <typename F>
        static void EachDynamicMotion(Registry&, DynamicMotionView& inView, F&& inFunc)
        {
            inView.run([&](flecs::iter& iter) -> void {
                while (iter.next()) {
                    const auto positions = iter.field<const Position>(0);
                    const auto velocities = iter.field<const Velocity>(1);
                    for (const auto i : iter) {
                        inFunc(positions[i], velocities[i]);
                    }
                }
            });
        }
    };

    template <typename Backend>
    static std::vector<typename Backend::Entity> CreateEntities(typename Backend::Registry& inRegistry, int64_t inCount)
    {
        std::vector<typename Backend::Entity> entities;
        entities.reserve(inCount);
        for (int64_t i = 0; i < inCount; i++) {
            entities.emplace_back(Backend::Create(inRegistry));
        }
        return entities;
    }

    template <typename Backend>
    static void AddMotionComponents(typename Backend::Registry& inRegistry, const std::vector<typename Backend::Entity>& inEntities, bool inAddPayload)
    {
        for (size_t i = 0; i < inEntities.size(); i++) {
            const auto value = static_cast<float>(i);
            Backend::template Emplace<Position>(inRegistry, inEntities[i], value, value + 1.0f, value + 2.0f);
            Backend::template Emplace<Velocity>(inRegistry, inEntities[i], 1.0f, 2.0f, 3.0f);
            if (inAddPayload) {
                Backend::template Emplace<Payload>(inRegistry, inEntities[i], static_cast<uint64_t>(i));
            }
        }
    }

    static void SetEntitiesProcessed(benchmark::State& inState, int64_t inEntityCount)
    {
        inState.SetItemsProcessed(inState.iterations() * inEntityCount);
    }

    template <typename Backend>
    static void EntityCreateDestroy(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        std::vector<typename Backend::Entity> entities;
        entities.reserve(entityCount);

        for (auto _ : state) {
            for (int64_t i = 0; i < entityCount; i++) {
                entities.emplace_back(Backend::Create(registry));
            }
            for (const auto entity : entities) {
                Backend::Destroy(registry, entity);
            }
            entities.clear();
            benchmark::ClobberMemory();
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ComponentAddRemove(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);

        for (auto _ : state) {
            for (const auto entity : entities) {
                Backend::template Emplace<Position>(registry, entity, 1.0f, 2.0f, 3.0f);
            }
            for (const auto entity : entities) {
                Backend::template Remove<Position>(registry, entity);
            }
            benchmark::ClobberMemory();
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ThreeComponentChurn(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);

        for (auto _ : state) {
            for (const auto entity : entities) {
                Backend::template Emplace<Position>(registry, entity, 1.0f, 2.0f, 3.0f);
            }
            for (const auto entity : entities) {
                Backend::template Emplace<Velocity>(registry, entity, 4.0f, 5.0f, 6.0f);
            }
            for (const auto entity : entities) {
                Backend::template Emplace<Health>(registry, entity, uint32_t { 75 }, uint32_t { 100 });
            }
            for (const auto entity : entities) {
                Backend::template Remove<Health>(registry, entity);
            }
            for (const auto entity : entities) {
                Backend::template Remove<Velocity>(registry, entity);
            }
            for (const auto entity : entities) {
                Backend::template Remove<Position>(registry, entity);
            }
            benchmark::ClobberMemory();
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ComponentGet(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);
        AddMotionComponents<Backend>(registry, entities, false);

        for (auto _ : state) {
            float sum = 0.0f;
            for (const auto entity : entities) {
                const auto& position = Backend::template Get<Position>(registry, entity);
                sum += position.x + position.y + position.z;
            }
            benchmark::DoNotOptimize(sum);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ViewConstruct(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);

        for (size_t i = 0; i < entities.size(); i++) {
            Backend::template Emplace<Position>(registry, entities[i], 1.0f, 2.0f, 3.0f);
            if (i % 2 == 0) {
                Backend::template Emplace<Velocity>(registry, entities[i], 4.0f, 5.0f, 6.0f);
            }
            if (i % 4 == 0) {
                Backend::template Emplace<Health>(registry, entities[i], uint32_t { 75 }, uint32_t { 100 });
            }
        }

        for (auto _ : state) {
            auto view = Backend::MakeFilteredMotionView(registry);
            benchmark::DoNotOptimize(view);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ViewIterate(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);
        AddMotionComponents<Backend>(registry, entities, false);
        auto view = Backend::MakeMotionView(registry);

        for (auto _ : state) {
            float sum = 0.0f;
            Backend::EachMotion(view, [&](const Position& position, const Velocity& velocity) -> void {
                sum += position.x * velocity.x + position.y * velocity.y + position.z * velocity.z;
            });
            benchmark::DoNotOptimize(sum);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ViewIterateWideArchetype(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);
        AddMotionComponents<Backend>(registry, entities, true);
        auto view = Backend::MakeMotionView(registry);

        for (auto _ : state) {
            float sum = 0.0f;
            Backend::EachMotion(view, [&](const Position& position, const Velocity& velocity) -> void {
                sum += position.x * velocity.x + position.y * velocity.y + position.z * velocity.z;
            });
            benchmark::DoNotOptimize(sum);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void ViewConstructAndIterate(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);
        AddMotionComponents<Backend>(registry, entities, false);

        for (auto _ : state) {
            float sum = 0.0f;
            auto view = Backend::MakeMotionView(registry);
            Backend::EachMotion(view, [&](const Position& position, const Velocity& velocity) -> void {
                sum += position.x * velocity.x + position.y * velocity.y + position.z * velocity.z;
            });
            benchmark::DoNotOptimize(sum);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void DynamicViewConstructAndIterate(benchmark::State& state)
    {
        const auto entityCount = state.range(0);
        typename Backend::Registry registry;
        const auto entities = CreateEntities<Backend>(registry, entityCount);
        AddMotionComponents<Backend>(registry, entities, false);

        for (auto _ : state) {
            float sum = 0.0f;
            auto view = Backend::MakeDynamicMotionView(registry);
            Backend::EachDynamicMotion(registry, view, [&](const Position& position, const Velocity& velocity) -> void {
                sum += position.x * velocity.x + position.y * velocity.y + position.z * velocity.z;
            });
            benchmark::DoNotOptimize(sum);
        }

        SetEntitiesProcessed(state, entityCount);
    }

    template <typename Backend>
    static void RegisterBenchmarkCase(std::string_view inCaseName, void (*inFunction)(benchmark::State&))
    {
        std::string name = "Runtime::ECSBenchmark::";
        name.append(inCaseName);
        name += "/";
        name.append(Backend::name);

        benchmark::RegisterBenchmark(name.c_str(), inFunction)
            ->Arg(smallEntityCount)
            ->Arg(mediumEntityCount)
            ->Arg(largeEntityCount);
    }

    template <typename Backend>
    static void RegisterBackendBenchmarks()
    {
        RegisterBenchmarkCase<Backend>("EntityCreateDestroy", &EntityCreateDestroy<Backend>);
        RegisterBenchmarkCase<Backend>("ComponentAddRemove", &ComponentAddRemove<Backend>);
        RegisterBenchmarkCase<Backend>("ThreeComponentChurn", &ThreeComponentChurn<Backend>);
        RegisterBenchmarkCase<Backend>("ComponentGet", &ComponentGet<Backend>);
        RegisterBenchmarkCase<Backend>("ViewConstruct", &ViewConstruct<Backend>);
        RegisterBenchmarkCase<Backend>("ViewIterate", &ViewIterate<Backend>);
        RegisterBenchmarkCase<Backend>("ViewIterateWideArchetype", &ViewIterateWideArchetype<Backend>);
        RegisterBenchmarkCase<Backend>("ViewConstructAndIterate", &ViewConstructAndIterate<Backend>);
        RegisterBenchmarkCase<Backend>("DynamicViewConstructAndIterate", &DynamicViewConstructAndIterate<Backend>);
    }

    const bool benchmarksRegistered = []() -> bool {
        RegisterBackendBenchmarks<ExplosionBackend>();
        RegisterBackendBenchmarks<EnTTBackend>();
        RegisterBackendBenchmarks<FlecsBackend>();
        return true;
    }();
}

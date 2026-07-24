//
// Created by johnk on 2026/7/12.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <Runtime/ECS.h>

namespace Runtime::ECSBenchmark {
    struct EClass(comp) Position final {
        EClassBody(Position)

        Position();
        Position(float inX, float inY, float inZ);

        float x;
        float y;
        float z;
    };

    struct EClass(comp) Velocity final {
        EClassBody(Velocity)

        Velocity();
        Velocity(float inX, float inY, float inZ);

        float x;
        float y;
        float z;
    };

    struct EClass(comp) Health final {
        EClassBody(Health)

        Health();
        Health(uint32_t inCurrent, uint32_t inMaximum);

        uint32_t current;
        uint32_t maximum;
    };

    struct EClass(comp) Payload final {
        EClassBody(Payload)

        Payload();
        explicit Payload(uint64_t inSeed);

        std::array<uint64_t, 16> values;
    };

    class EClass() EmptySystem final : public System {
    public:
        EPolyDerivedClassBody(EmptySystem)

        explicit EmptySystem(ECRegistry& inRegistry, const SystemSetupContext& inContext);
        ~EmptySystem() override;

        void Tick(float inDeltaTimeSeconds) override;

    private:
        uint64_t tickCount;
    };
}

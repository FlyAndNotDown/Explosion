//
// Created by johnk on 2025/3/13.
//

#pragma once

#include <Runtime/Api.h>
#include <Runtime/ECS.h>
#include <Runtime/Meta.h>

namespace Runtime {
    class RUNTIME_API EClass() PlayerSystem final : public System {
        EClassBody(PlayerSystem)

    public:
        NonCopyable(PlayerSystem)
        NonMovable(PlayerSystem)

        PlayerSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext);
        ~PlayerSystem() override;

    private:
        Entity CreateLocalPlayer();

        uint8_t activeLocalPlayerNum;
    };
}

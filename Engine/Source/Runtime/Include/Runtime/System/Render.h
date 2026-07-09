//
// Created by johnk on 2025/3/4.
//

#pragma once

#include <Common/Math/Rect.h>
#include <Render/RenderModule.h>
#include <Render/View.h>
#include <Runtime/Client.h>
#include <Runtime/ECS.h>
#include <Runtime/RenderSurface.h>
#include <Runtime/Api.h>

namespace Runtime {
    class RUNTIME_API EClass() RenderSystem final : public System {
        EPolyClassBody(RenderSystem)

    public:
        explicit RenderSystem(ECRegistry& inRegistry, const SystemSetupContext& inContext);
        ~RenderSystem() override;

        NonCopyable(RenderSystem)
        NonMovable(RenderSystem)

        void Tick(float inDeltaTimeSeconds) override;

    private:
        static Common::URect GetPlayerViewport(uint32_t inWidth, uint32_t inHeight, uint8_t inPlayerNum, uint8_t inPlayerIndex);
        Render::View BuildViewForCamera(Entity inEntity) const;
        std::vector<Render::View> BuildViews() const;

        Render::RenderModule& renderModule;
        Client* client;
        RHI::Fence* lastFrameFence;
    };
}

//
// Created by johnk on 2025/2/18.
//

#pragma once

#include <RHI/Texture.h>
#include <RHI/Synchronous.h>
#include <Runtime/Api.h>

namespace Runtime {
    class Client;

    struct RUNTIME_API PresentInfo {
        PresentInfo();

        RHI::Texture* backTexture;
        RHI::Semaphore* imageReadySemaphore;
        RHI::Semaphore* renderFinishedSemaphore;
    };

    class RUNTIME_API Viewport {
    public:
        virtual ~Viewport();

        virtual Client& GetClient() = 0;
        // called on render thread after the previous frame's gpu work finished, acquires the next back texture and
        // returns the semaphores guarding it
        virtual PresentInfo GetNextPresentInfo() = 0;
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual void Resize(uint32_t inWidth, uint32_t inHeight) = 0;
        // called on render thread after rendering finished, implementations present the back texture acquired by the
        // matching GetNextPresentInfo() call, waiting inPresentInfo.renderFinishedSemaphore
        virtual void Present(const PresentInfo& inPresentInfo) = 0;
        // TODO mouse keyboard inputs etc.

    protected:
        Viewport();
    };
}

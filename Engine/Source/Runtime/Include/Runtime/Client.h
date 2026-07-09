//
// Created by johnk on 2025/2/18.
//

#pragma once

#include <Runtime/Api.h>
#include <Runtime/RenderSurface.h>

namespace Runtime {
    class World;

    class RUNTIME_API Client {
    public:
        virtual ~Client();

        virtual World& GetWorld() = 0;
        virtual RenderSurface* GetRenderSurface() = 0;

    protected:
        Client();
    };
}

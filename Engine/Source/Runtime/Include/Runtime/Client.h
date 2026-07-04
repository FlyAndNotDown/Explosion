//
// Created by johnk on 2025/2/18.
//

#pragma once

#include <Runtime/Viewport.h>

namespace Runtime {
    class World;

    class Client {
    public:
        virtual ~Client();

        virtual World& GetWorld() = 0;
        // maybe nullptr when the client has no active viewport (e.g. headless world or editor viewport closed),
        // rendering is skipped in that case
        virtual Viewport* GetViewport() = 0;

    protected:
        Client();
    };
}

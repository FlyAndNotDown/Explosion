//
// Created by johnk on 2025/2/19.
//

#pragma once

#include <GLFW/glfw3.h>
#if PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif PLATFORM_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include <Render/RenderModule.h>
#include <Runtime/Window.h>

namespace Launch {
    struct GameWindowDesc {
        std::string title;
        uint32_t width;
        uint32_t height;
    };

    class GameWindow final : public Runtime::Window {
    public:
        explicit GameWindow(const GameWindowDesc& inDesc);
        ~GameWindow() override;

        void Resize(uint32_t inWidth, uint32_t inHeight) override;

        bool ShouldClose() const;
        void PollEvents() const;
        void WaitRenderingIdle() const;

    private:
        Render::RenderModule& renderModule;
        GLFWwindow* window;
    };
}

//
// Created by johnk on 2025/2/19.
//

#include <Launch/GameWindow.h>
#include <Runtime/Engine.h>

namespace Launch::Internal {
    static void* GetGlfwPlatformWindow(GLFWwindow* inWindow)
    {
#if PLATFORM_WINDOWS
        return glfwGetWin32Window(inWindow);
#elif PLATFORM_MACOS
        return glfwGetCocoaView(inWindow);
#else
        Unimplement();
        return nullptr;
#endif
    }
}

namespace Launch {
    GameWindow::GameWindow(const GameWindowDesc& inDesc)
        : Runtime::Window(*Runtime::EngineHolder::Get().GetRenderModule().GetDevice())
        , renderModule(Runtime::EngineHolder::Get().GetRenderModule())
        , window(nullptr)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(
            static_cast<int>(inDesc.width),
            static_cast<int>(inDesc.height),
            inDesc.title.c_str(),
            nullptr,
            nullptr);
        Assert(window != nullptr);

        InitializeSurface(
            GetDevice().CreateSurface(RHI::SurfaceCreateInfo(Internal::GetGlfwPlatformWindow(window))),
            inDesc.width,
            inDesc.height);
    }

    GameWindow::~GameWindow()
    {
        WaitRenderingIdle();
        DestroySurface();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void GameWindow::Resize(uint32_t inWidth, uint32_t inHeight)
    {
        glfwSetWindowSize(window, static_cast<int>(inWidth), static_cast<int>(inHeight));
        WaitRenderingIdle();
        Runtime::Window::Resize(inWidth, inHeight);
    }

    bool GameWindow::ShouldClose() const
    {
        return static_cast<bool>(glfwWindowShouldClose(window));
    }

    void GameWindow::PollEvents() const
    {
        glfwPollEvents();
    }

    void GameWindow::WaitRenderingIdle() const
    {
        renderModule.GetRenderThread().Flush();
        Runtime::Window::WaitRenderingIdle();
    }
}

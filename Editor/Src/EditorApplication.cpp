//
// Created by johnk on 2026/7/7.
//

#include <algorithm>
#include <format>
#include <tuple>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <Common/Time.h>
#include <Core/Paths.h>
#include <Editor/EditorApplication.h>
#include <Editor/Utils/ImGuiCompatibility.h>
#include <Runtime/Engine.h>

namespace Editor::Internal {
    constexpr uint32_t defaultWindowWidth = 1600;
    constexpr uint32_t defaultWindowHeight = 900;
    constexpr uint32_t projectHubWindowWidth = 560;
    constexpr uint32_t projectHubWindowHeight = 720;
    constexpr float defaultFontSize = 15.0f;
    constexpr uint32_t defaultSceneWidth = 1280;
    constexpr uint32_t defaultSceneHeight = 720;
    constexpr RHI::PixelFormat sceneColorFormat = RHI::PixelFormat::rgba8Unorm;
    constexpr const char* editorDockSpaceName = "EditorDockSpace";
    constexpr const char* editorDockSpaceId = "EditorDockSpaceV2";

    static Runtime::CanvasDesc CreateSceneRenderCanvasDesc()
    {
        Runtime::CanvasDesc result;
        result.width = defaultSceneWidth;
        result.height = defaultSceneHeight;
        result.format = sceneColorFormat;
        result.debugName = "editorSceneColor";
        return result;
    }

    static std::string MakeWindowTitle(const EditorApplicationDesc& inDesc)
    {
        if (inDesc.mode == EditorApplicationMode::projectHub) {
            return "Explosion Project Hub";
        }
        const std::string projectName = Core::Paths::HasSetGameRoot()
            ? Core::Paths::GameRootDir().DirName()
            : "Untitled";
        return std::format("{} - Explosion Editor", projectName);
    }

    static uint32_t GetInitialWindowWidth(const EditorApplicationDesc& inDesc)
    {
        return inDesc.mode == EditorApplicationMode::projectHub ? projectHubWindowWidth : defaultWindowWidth;
    }

    static uint32_t GetInitialWindowHeight(const EditorApplicationDesc& inDesc)
    {
        return inDesc.mode == EditorApplicationMode::projectHub ? projectHubWindowHeight : defaultWindowHeight;
    }

    static void SetDarkStyle()
    {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.ChildRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.PopupRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
    }

    static void BuildInitialEditorDockLayout(ImGuiID inDockSpaceId, const ImVec2& inSize)
    {
        if (ImGui::DockBuilderGetNode(inDockSpaceId) != nullptr) {
            return;
        }

        ImGui::DockBuilderAddNode(inDockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(inDockSpaceId, inSize);

        ImGuiID sceneNodeId = 0;
        ImGuiID rightColumnNodeId = 0;
        ImGuiID outlinerNodeId = 0;
        ImGuiID inspectorNodeId = 0;
        ImGuiID logNodeId = 0;
        ImGui::DockBuilderSplitNode(inDockSpaceId, ImGuiDir_Right, 0.25f, &rightColumnNodeId, &sceneNodeId);
        ImGui::DockBuilderSplitNode(sceneNodeId, ImGuiDir_Down, 0.25f, &logNodeId, &sceneNodeId);
        ImGui::DockBuilderSplitNode(rightColumnNodeId, ImGuiDir_Down, 0.5f, &inspectorNodeId, &outlinerNodeId);

        ImGui::DockBuilderDockWindow("Scene", sceneNodeId);
        ImGui::DockBuilderDockWindow("Outliner", outlinerNodeId);
        ImGui::DockBuilderDockWindow("Inspector", inspectorNodeId);
        ImGui::DockBuilderDockWindow("Log", logNodeId);
        ImGui::DockBuilderFinish(inDockSpaceId);
    }

    static ImDrawData* CloneDrawData(const ImDrawData& inDrawData)
    {
        auto* result = IM_NEW(ImDrawData)();
        result->Valid = inDrawData.Valid;
        result->CmdListsCount = inDrawData.CmdListsCount;
        result->TotalIdxCount = inDrawData.TotalIdxCount;
        result->TotalVtxCount = inDrawData.TotalVtxCount;
        result->DisplayPos = inDrawData.DisplayPos;
        result->DisplaySize = inDrawData.DisplaySize;
        result->FramebufferScale = inDrawData.FramebufferScale;
        result->OwnerViewport = inDrawData.OwnerViewport;
        result->Textures = inDrawData.Textures;
        result->CmdLists.resize(inDrawData.CmdListsCount);
        for (int i = 0; i < inDrawData.CmdListsCount; i++) {
            result->CmdLists[i] = inDrawData.CmdLists[i]->CloneOutput();
        }
        return result;
    }
}

namespace Editor {
    EditorApplication::EditorApplication(EditorApplicationDesc inDesc)
        : desc(std::move(inDesc))
        , editorFrame()
        , lastFrameSeconds(Common::TimePoint::Now().ToSeconds())
        , lastCursorX(0.0)
        , lastCursorY(0.0)
        , hasLastCursor(false)
        , requestQuit(false)
    {
        InitializeImGui();
        window = Common::MakeUnique<EditorWindow>(
            EditorWindowDesc {
                .title = Internal::MakeWindowTitle(desc),
                .width = Internal::GetInitialWindowWidth(desc),
                .height = Internal::GetInitialWindowHeight(desc)
            });
        InstallCallbacks();

        if (desc.mode == EditorApplicationMode::editor) {
            context = Common::MakeUnique<EditorContext>();
            auto& sceneClient = context->GetSceneClient();
            sceneClient.SetEditorWindow(*window);
            sceneRenderCanvas = Common::MakeUnique<Runtime::Canvas>(window->GetDevice(), Internal::CreateSceneRenderCanvasDesc());
            sceneClient.SetRenderSurface(sceneRenderCanvas.Get());
            sceneClient.OpenProjectLevel();
        } else {
            projectHubFrame = Common::MakeUnique<ProjectHubFrame>();
        }
    }

    EditorApplication::~EditorApplication()
    {
        if (context != nullptr) {
            context->GetSceneClient().SetRenderSurface(nullptr);
        }
        if (window != nullptr) {
            window->WaitRenderingIdle();
        }
        sceneRenderCanvas.Reset();
        context.Reset();
        projectHubFrame.Reset();
        window.Reset();
        ShutdownImGui();
    }

    int EditorApplication::Run()
    {
        while (!window->ShouldClose()) {
            window->PollEvents();
            window->RecreateSwapChainIfNeeded();
            const float deltaSeconds = NextDeltaSeconds();
            BeginImGuiFrame(deltaSeconds);

            if (desc.mode == EditorApplicationMode::editor) {
                RenderEditorFrame(deltaSeconds);
            } else {
                RenderProjectHubFrame();
            }
        }
        return 0;
    }

    void EditorApplication::InitializeImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.BackendPlatformName = "ExplosionGLFW";
        io.BackendRendererName = "ExplosionRHI";
        if (desc.mode == EditorApplicationMode::editor) {
            const Common::Path iniFilePath = Core::Paths::GameCacheDir() / "Editor" / "Layout" / "Main.ini";
            iniFilePath.Parent().MakeDir();
            imguiIniFilename = iniFilePath.String();
            io.IniFilename = imguiIniFilename.c_str();
        } else {
            io.IniFilename = nullptr;
        }
        ImFontConfig defaultFontConfig;
        defaultFontConfig.SizePixels = Internal::defaultFontSize;
        io.FontDefault = io.Fonts->AddFontDefaultVector(&defaultFontConfig);
        Internal::SetDarkStyle();
    }

    void EditorApplication::ShutdownImGui()
    {
        ImGui::DestroyContext();
    }

    void EditorApplication::InstallCallbacks()
    {
        GLFWwindow* nativeWindow = window->GetNativeWindow();
        glfwSetWindowUserPointer(nativeWindow, this);
        glfwSetKeyCallback(nativeWindow, [](GLFWwindow* inWindow, int inKey, int inScancode, int inAction, int inMods) -> void {
            static_cast<EditorApplication*>(glfwGetWindowUserPointer(inWindow))->HandleKey(inKey, inScancode, inAction, inMods);
        });
        glfwSetCharCallback(nativeWindow, [](GLFWwindow* inWindow, unsigned int inCodepoint) -> void {
            static_cast<EditorApplication*>(glfwGetWindowUserPointer(inWindow))->HandleChar(inCodepoint);
        });
        glfwSetMouseButtonCallback(nativeWindow, [](GLFWwindow* inWindow, int inButton, int inAction, int inMods) -> void {
            static_cast<EditorApplication*>(glfwGetWindowUserPointer(inWindow))->HandleMouseButton(inButton, inAction, inMods);
        });
        glfwSetCursorPosCallback(nativeWindow, [](GLFWwindow* inWindow, double inX, double inY) -> void {
            static_cast<EditorApplication*>(glfwGetWindowUserPointer(inWindow))->HandleCursor(inX, inY);
        });
        glfwSetScrollCallback(nativeWindow, [](GLFWwindow* inWindow, double inXOffset, double inYOffset) -> void {
            static_cast<EditorApplication*>(glfwGetWindowUserPointer(inWindow))->HandleScroll(inXOffset, inYOffset);
        });
    }

    void EditorApplication::BeginImGuiFrame(float inDeltaSeconds) const
    {
        int windowWidth = 0;
        int windowHeight = 0;
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetWindowSize(window->GetNativeWindow(), &windowWidth, &windowHeight);
        glfwGetFramebufferSize(window->GetNativeWindow(), &framebufferWidth, &framebufferHeight);

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        io.DisplayFramebufferScale = ImVec2(
            windowWidth > 0 ? static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth) : 1.0f,
            windowHeight > 0 ? static_cast<float>(framebufferHeight) / static_cast<float>(windowHeight) : 1.0f);
        io.DeltaTime = std::max(inDeltaSeconds, 1.0f / 1000.0f);
        ImGui::NewFrame();
    }

    void EditorApplication::DrawDockSpace() const
    {
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(mainViewport->WorkPos);
        ImGui::SetNextWindowSize(mainViewport->WorkSize);
        ImGui::SetNextWindowViewport(mainViewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(Internal::editorDockSpaceName, nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        const ImGuiID dockSpaceId = ImGui::GetID(Internal::editorDockSpaceId);
        Internal::BuildInitialEditorDockLayout(dockSpaceId, mainViewport->WorkSize);
        ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();
    }

    void EditorApplication::RenderEditorFrame(float inDeltaSeconds)
    {
        DrawDockSpace();
        editorFrame.Render(*context, *sceneRenderCanvas, requestQuit);
        if (requestQuit) {
            window->RequestClose();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            context->GetSceneClient().SaveLevel();
        }

        ImGui::Render();
        window->SetDrawData(Internal::CloneDrawData(*ImGui::GetDrawData()));
        context->GetSceneClient().TickEditorCamera(inDeltaSeconds);
        Runtime::EngineHolder::Get().Tick(inDeltaSeconds);
        window->RenderPendingUi();
    }

    void EditorApplication::RenderProjectHubFrame()
    {
        projectHubFrame->Render(*window, desc.rhiType);
        ImGui::Render();
        Runtime::EngineHolder::Get().Tick(ImGui::GetIO().DeltaTime);
        window->RenderUiOnly(*ImGui::GetDrawData());
    }

    void EditorApplication::HandleKey(int inKey, int inScancode, int inAction, int inMods)
    {
        std::ignore = inScancode;
        ImGuiCompatibility::UpdateKeyModifiers(inMods);

        ImGuiIO& io = ImGui::GetIO();
        const ImGuiKey imguiKey = ImGuiCompatibility::ToImGuiKey(inKey);
        if (imguiKey != ImGuiKey_None) {
            io.AddKeyEvent(imguiKey, inAction != GLFW_RELEASE);
            io.SetKeyEventNativeData(imguiKey, inKey, inScancode);
        }

        if (context != nullptr) {
            auto& sceneClient = context->GetSceneClient();
            if (inKey == GLFW_KEY_ESCAPE && inAction == GLFW_PRESS) {
                sceneClient.EndCameraLook();
            }
            if (inAction == GLFW_PRESS || inAction == GLFW_RELEASE) {
                const bool canRouteToScene = sceneClient.IsCameraLooking() || (!io.WantCaptureKeyboard && sceneClient.IsSceneHovered());
                if (canRouteToScene) {
                    sceneClient.OnKey(inKey, inAction == GLFW_PRESS);
                }
            }
        }
    }

    void EditorApplication::HandleChar(uint32_t inCodepoint) const
    {
        ImGui::GetIO().AddInputCharacter(inCodepoint);
    }

    void EditorApplication::HandleMouseButton(int inButton, int inAction, int inMods)
    {
        ImGuiCompatibility::UpdateKeyModifiers(inMods);
        if (const auto imguiMouseButton = ImGuiCompatibility::ToImGuiMouseButton(inButton)) {
            ImGui::GetIO().AddMouseButtonEvent(*imguiMouseButton, inAction == GLFW_PRESS);
        }

        if (context == nullptr || inButton != GLFW_MOUSE_BUTTON_RIGHT) {
            return;
        }
        auto& sceneClient = context->GetSceneClient();
        if (inAction == GLFW_PRESS && sceneClient.IsSceneHovered()) {
            hasLastCursor = false;
            sceneClient.BeginCameraLook();
        } else if (inAction == GLFW_RELEASE) {
            sceneClient.EndCameraLook();
        }
    }

    void EditorApplication::HandleCursor(double inX, double inY)
    {
        ImGui::GetIO().AddMousePosEvent(static_cast<float>(inX), static_cast<float>(inY));
        if (context != nullptr) {
            auto& sceneClient = context->GetSceneClient();
            if (sceneClient.IsCameraLooking() && hasLastCursor) {
                sceneClient.AddCameraLookDelta(static_cast<float>(inX - lastCursorX), static_cast<float>(inY - lastCursorY));
            }
        }
        lastCursorX = inX;
        lastCursorY = inY;
        hasLastCursor = true;
    }

    void EditorApplication::HandleScroll(double inXOffset, double inYOffset) const
    {
        ImGui::GetIO().AddMouseWheelEvent(static_cast<float>(inXOffset), static_cast<float>(inYOffset));
    }

    float EditorApplication::NextDeltaSeconds()
    {
        const double now = Common::TimePoint::Now().ToSeconds();
        const float deltaSeconds = static_cast<float>(now - lastFrameSeconds);
        lastFrameSeconds = now;
        return deltaSeconds;
    }
}

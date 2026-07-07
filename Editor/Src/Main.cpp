//
// Created by johnk on 2024/3/31.
//

#include <Core/Cmdline.h>
#include <Editor/EditorApplication.h>
#include <Editor/EditorLog.h>
#include <Runtime/Engine.h>

static Core::CmdlineArgValue<std::string> caRhiType(
    "rhiType", "-rhi", RHI::GetPlatformDefaultRHIAbbrString(),
    "rhi abbr string, can be 'dx12' or 'vulkan'");

static Core::CmdlineArgValue<std::string> caProjectRoot(
    "projectRoot", "-project", "",
    "project root path");

static Editor::EditorApplicationMode GetAppMode()
{
    return caProjectRoot.GetValue().empty()
        ? Editor::EditorApplicationMode::projectHub
        : Editor::EditorApplicationMode::editor;
}

static void InitializeEngine()
{
    Editor::EditorLogStream::Get();

    Runtime::EngineInitParams params {};
    params.logToFile = true;
    params.gameRoot = caProjectRoot.GetValue();
    params.rhiType = caRhiType.GetValue();
    Runtime::EngineHolder::Load("Editor", params);
}

int main(int argc, char* argv[])
{
    Core::Cli::Get().Parse(argc, argv);

    InitializeEngine();
    const Editor::EditorApplicationDesc appDesc {
        .mode = GetAppMode(),
        .rhiType = caRhiType.GetValue(),
        .projectRoot = caProjectRoot.GetValue()
    };
    int result = 0;
    {
        Editor::EditorApplication application(appDesc);
        result = application.Run();
    }
    Runtime::EngineHolder::Unload();
    return result;
}

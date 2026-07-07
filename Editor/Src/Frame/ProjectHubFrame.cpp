//
// Created by johnk on 2026/7/8.
//

#include <cstdlib>
#include <format>
#include <ranges>
#include <string_view>
#include <tuple>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <Common/Result.h>
#include <Common/Serialization.h>
#include <Common/Template.h>
#include <Core/EngineVersion.h>
#include <Core/Paths.h>
#include <Editor/EditorLog.h>
#include <Editor/EditorWindow.h>
#include <Editor/Frame/ProjectHubFrame.h>

namespace Editor::Internal {
    constexpr std::string_view templateFileExtension = ".tpl";
    constexpr std::string_view cmakeMinVersion = "3.25";

    static Common::Path StripTemplateExtension(const Common::Path& inRelativePath)
    {
        const std::string str = inRelativePath.String();
        return { str.substr(0, str.size() - templateFileExtension.size()) };
    }

    static Common::Result<void, std::string> RenderProjectTemplate(
        const Common::Path& inTemplateDir,
        const Common::Path& inProjectDir,
        const std::string& inProjectName)
    {
        Common::TemplateEngine templateEngine;
        templateEngine
            .Set("projectName", inProjectName)
            .Set("cmakeMinVersion", std::string(cmakeMinVersion));

        Common::Result<void, std::string> result = Common::Ok();
        inTemplateDir.TraverseRecurse([&](const Common::Path& inPath) -> bool {
            if (inPath.IsDirectory()) {
                return true;
            }

            const Common::Path relativePath = inPath.Relative(inTemplateDir);
            const std::string extension = inPath.Extension();
            const bool isTemplate = std::string_view(extension) == templateFileExtension;
            const Common::Path dstPath = inProjectDir / (isTemplate ? StripTemplateExtension(relativePath) : relativePath);
            dstPath.Parent().MakeDir();

            if (!isTemplate) {
                inPath.CopyTo(dstPath);
                return true;
            }
            if (auto renderResult = templateEngine.RenderFileTo(inPath.String(), dstPath.String());
                renderResult.IsErr()) {
                result = Common::Err(renderResult.Error());
                return false;
            }
            return true;
        });
        return result;
    }

    static std::string LaunchCommand(const std::string& inExecutable, const std::string& inProjectPath, const std::string& inRhiType)
    {
#if PLATFORM_WINDOWS
        return std::format("start \"\" \"{}\" -project \"{}\" -rhi {}", inExecutable, inProjectPath, inRhiType);
#else
        return std::format("\"{}\" -project \"{}\" -rhi {} &", inExecutable, inProjectPath, inRhiType);
#endif
    }
}

namespace Editor {
    ProjectHubFrame::ProjectHubFrame()
        : recentProjectsFile(Core::Paths::EngineCacheDir() / "Editor" / "ProjectHub" / "RecentProjects.json")
        , engineVersion(std::format("v{}.{}.{}", ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH))
        , projectName()
        , directory()
        , selectedTemplateIndex(0)
        , lastCreatedProjectPath()
    {
        const Common::Path projectTemplatesRoot = Core::Paths::EngineResDir() / "Editor" / "ProjectTemplates";
        (void) projectTemplatesRoot.Traverse([this](const Common::Path& inPath) -> bool {
            if (inPath.IsFile()) {
                return true;
            }
            projectTemplates.emplace_back(ProjectTemplateInfo { inPath.DirName(), inPath.String() });
            return true;
        });

        if (recentProjectsFile.Exists()) {
            Common::JsonDeserializeFromFile(recentProjectsFile.String(), recentProjects);
        }
    }

    ProjectHubFrame::~ProjectHubFrame()
    {
        SaveRecentProjects();
    }

    void ProjectHubFrame::Render(EditorWindow& inWindow, const std::string& inRhiType)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Project Hub", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextUnformatted("Explosion Editor");
        ImGui::SameLine();
        ImGui::TextDisabled("%s", engineVersion.c_str());
        ImGui::Separator();

        ImGui::Columns(2, "ProjectHubColumns", true);
        ImGui::TextUnformatted("Recent Projects");
        ImGui::BeginChild("RecentProjects", ImVec2(0.0f, -1.0f), true);
        for (const auto& project : std::views::reverse(recentProjects)) {
            ImGui::PushID(project.path.c_str());
            if (ImGui::Selectable(project.name.c_str())) {
                OpenProject(inWindow, project.path, inRhiType);
            }
            ImGui::TextDisabled("%s", project.path.c_str());
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::NextColumn();
        ImGui::TextUnformatted("Create Project");
        ImGui::InputText("Name", &projectName);
        ImGui::InputText("Directory", &directory);

        const char* selectedTemplate = projectTemplates.empty() ? "None" : projectTemplates[selectedTemplateIndex].name.c_str();
        if (ImGui::BeginCombo("Template", selectedTemplate)) {
            for (int i = 0; i < static_cast<int>(projectTemplates.size()); i++) {
                const bool selected = i == selectedTemplateIndex;
                if (ImGui::Selectable(projectTemplates[i].name.c_str(), selected)) {
                    selectedTemplateIndex = i;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Create")) {
            const CreateProjectResult result = CreateProject();
            lastCreatedProjectPath = result.success ? result.projectPath : std::string();
            statusMessage = result.success ? std::format("Created {}", result.projectPath) : result.error;
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Created") && !lastCreatedProjectPath.empty()) {
            OpenProject(inWindow, lastCreatedProjectPath, inRhiType);
        }
        if (!statusMessage.empty()) {
            ImGui::TextWrapped("%s", statusMessage.c_str());
        }
        ImGui::Columns(1);
        ImGui::End();
    }

    CreateProjectResult ProjectHubFrame::CreateProject()
    {
        if (projectName.empty() || directory.empty() || projectTemplates.empty()) {
            return { .success = false, .error = "Project name, directory and template must not be empty.", .projectPath = {} };
        }

        const std::string templatePath = projectTemplates[selectedTemplateIndex].path;
        const Common::Path templateDir(templatePath);
        if (!templateDir.Exists() || !templateDir.IsDirectory()) {
            return { .success = false, .error = std::format("Project template '{}' does not exist.", templatePath), .projectPath = {} };
        }

        const Common::Path projectDir = Common::Path(directory) / projectName;
        if (projectDir.Exists()) {
            return { .success = false, .error = std::format("Target directory '{}' already exists.", projectDir.String()), .projectPath = {} };
        }

        if (const auto result = Internal::RenderProjectTemplate(templateDir, projectDir, projectName);
            result.IsErr()) {
            return { .success = false, .error = result.Error(), .projectPath = {} };
        }

        recentProjects.emplace_back(RecentProjectInfo { projectName, projectDir.String() });
        SaveRecentProjects();
        LogInfo(ProjectHub, "created project '{}' at '{}'", projectName, projectDir.String());
        return { .success = true, .error = {}, .projectPath = projectDir.String() };
    }

    void ProjectHubFrame::OpenProject(EditorWindow& inWindow, const std::string& inProjectPath, const std::string& inRhiType)
    {
        const Common::Path projectDir(inProjectPath);
        if (!projectDir.Exists() || !projectDir.IsDirectory()) {
            statusMessage = std::format("Project '{}' does not exist.", inProjectPath);
            return;
        }

        TouchRecentProject(inProjectPath);
        SaveRecentProjects();

        const std::string command = Internal::LaunchCommand(Core::Paths::ExecutablePath().String(), inProjectPath, inRhiType);
        std::ignore = std::system(command.c_str());
        inWindow.RequestClose();
    }

    void ProjectHubFrame::SaveRecentProjects() const
    {
        recentProjectsFile.Parent().MakeDir();
        Common::JsonSerializeToFile(recentProjectsFile.String(), recentProjects);
    }

    void ProjectHubFrame::TouchRecentProject(const std::string& inProjectPath)
    {
        std::string touchedProjectName = Common::Path(inProjectPath).DirName();
        if (const auto iter = std::ranges::find_if(recentProjects, [&](const RecentProjectInfo& info) -> bool { return info.path == inProjectPath; });
            iter != recentProjects.end()) {
            touchedProjectName = iter->name;
            recentProjects.erase(iter);
        }
        recentProjects.emplace_back(RecentProjectInfo { touchedProjectName, inProjectPath });
    }
}

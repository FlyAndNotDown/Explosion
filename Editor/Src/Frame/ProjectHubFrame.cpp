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
#include <Core/Paths.h>
#include <Editor/EditorLog.h>
#include <Editor/EditorWindow.h>
#include <Editor/Frame/ProjectHubFrame.h>
#include <Editor/Utils/PlatformUtils.h>
#include <Editor/Widget/IconWidgets.h>
#include <Editor/Widget/TablerIcons.h>

namespace Editor::ProjectHub::Internal {
    constexpr std::string_view templateFileExtension = ".tpl";
    constexpr std::string_view cmakeMinVersion = "3.25";
    constexpr float actionButtonHeight = 58.0f;
    constexpr float recentProjectHeight = 68.0f;
    constexpr float contentSpacing = 12.0f;
    const ImVec4 windowBackground(0.055f, 0.063f, 0.078f, 1.0f);
    const ImVec4 cardBackground(0.09f, 0.102f, 0.125f, 1.0f);
    const ImVec4 cardHovered(0.125f, 0.145f, 0.18f, 1.0f);
    const ImVec4 accent(0.25f, 0.49f, 0.96f, 1.0f);
    const ImVec4 accentHovered(0.31f, 0.55f, 1.0f, 1.0f);
    const ImVec4 secondaryButton(0.12f, 0.137f, 0.17f, 1.0f);
    const ImVec4 secondaryButtonHovered(0.16f, 0.18f, 0.22f, 1.0f);

    static bool RenderActionButton(const char* inLabel, const ImVec2& inSize, const ImVec4& inColor, const ImVec4& inHoveredColor)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, inColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, inHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, inHoveredColor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
        const bool clicked = ImGui::Button(inLabel, inSize);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        return clicked;
    }

    static bool RenderRecentProject(const RecentProjectInfo& inProject)
    {
        ImGui::PushID(inProject.path.c_str());
        const ImVec2 cardMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##RecentProject", ImVec2(ImGui::GetContentRegionAvail().x, recentProjectHeight));
        const ImVec2 cardMax = ImGui::GetItemRectMax();
        const bool hovered = ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(cardMin, cardMax, ImGui::GetColorU32(hovered ? cardHovered : cardBackground), 8.0f);
        drawList->AddText(ImVec2(cardMin.x + 16.0f, cardMin.y + 26.0f), ImGui::GetColorU32(accent), Icons::Tabler::folder);
        drawList->PushClipRect(ImVec2(cardMin.x + 46.0f, cardMin.y), ImVec2(cardMax.x - 38.0f, cardMax.y), true);
        drawList->AddText(ImVec2(cardMin.x + 46.0f, cardMin.y + 13.0f), ImGui::GetColorU32(ImGuiCol_Text), inProject.name.c_str());
        drawList->AddText(ImVec2(cardMin.x + 46.0f, cardMin.y + 38.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), inProject.path.c_str());
        drawList->PopClipRect();
        drawList->AddText(ImVec2(cardMax.x - 25.0f, cardMin.y + 26.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), Icons::Tabler::chevronRight);

        ImGui::PopID();
        return ImGui::IsItemClicked();
    }

    static Common::Path StripTemplateExtension(const Common::Path& inRelativePath)
    {
        const std::string str = inRelativePath.String();
        return { str.substr(0, str.size() - templateFileExtension.size()) };
    }

    static Common::Result<void, std::string> RenderProjectTemplate(const Common::Path& inTemplateDir, const Common::Path& inProjectDir, const std::string& inProjectName)
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
        , projectName()
        , directory()
        , selectedTemplateIndex(0)
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
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ProjectHub::Internal::windowBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(26.0f, 24.0f));
        ImGui::Begin(
            "##ProjectHub",
            nullptr,
            ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        RenderActionBar(inWindow, inRhiType);
        ImGui::Dummy(ImVec2(0.0f, 22.0f));
        RenderRecentProjects(inWindow, inRhiType);
        RenderCreateProjectPopup(inWindow, inRhiType);
        ImGui::End();
    }

    void ProjectHubFrame::RenderActionBar(EditorWindow& inWindow, const std::string& inRhiType)
    {
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;
        const std::string openLabel = Widgets::Label(Icons::Tabler::folderOpen, "Open");
        if (ProjectHub::Internal::RenderActionButton(openLabel.c_str(), ImVec2(buttonWidth, ProjectHub::Internal::actionButtonHeight), ProjectHub::Internal::accent, ProjectHub::Internal::accentHovered)) {
            if (const auto selectedDirectory = PlatformUtils::SelectDirectory("Open Explosion Project")) {
                OpenProject(inWindow, *selectedDirectory, inRhiType);
            }
        }

        ImGui::SameLine();
        const std::string createLabel = Widgets::Label(Icons::Tabler::plus, "Create");
        if (ProjectHub::Internal::RenderActionButton(createLabel.c_str(), ImVec2(buttonWidth, ProjectHub::Internal::actionButtonHeight), ProjectHub::Internal::secondaryButton, ProjectHub::Internal::secondaryButtonHovered)) {
            statusMessage.clear();
            ImGui::OpenPopup("##CreateProjectPopup");
        }
    }

    void ProjectHubFrame::RenderRecentProjects(EditorWindow& inWindow, const std::string& inRhiType)
    {
        const std::string recentProjectsLabel = Widgets::Label(Icons::Tabler::folder, "Recent projects");
        ImGui::TextUnformatted(recentProjectsLabel.c_str());
        const std::string projectCount = std::to_string(recentProjects.size());
        ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(projectCount.c_str()).x);
        ImGui::TextDisabled("%s", projectCount.c_str());
        ImGui::Dummy(ImVec2(0.0f, 7.0f));

        if (!statusMessage.empty()) {
            const std::string statusLabel = Widgets::Label(Icons::Tabler::circleX, statusMessage);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.42f, 0.42f, 1.0f));
            ImGui::TextWrapped("%s", statusLabel.c_str());
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0.0f, 7.0f));
        }

        ImGui::BeginChild("##RecentProjects", ImVec2(0.0f, 0.0f), false);
        std::string projectToOpen;
        for (const auto& project : std::views::reverse(recentProjects)) {
            if (ProjectHub::Internal::RenderRecentProject(project)) {
                projectToOpen = project.path;
            }
            ImGui::Dummy(ImVec2(0.0f, ProjectHub::Internal::contentSpacing));
        }

        if (recentProjects.empty()) {
            const ImVec2 emptyMin = ImGui::GetCursorScreenPos();
            const ImVec2 emptyMax(emptyMin.x + ImGui::GetContentRegionAvail().x, emptyMin.y + 118.0f);
            ImGui::GetWindowDrawList()->AddRectFilled(emptyMin, emptyMax, ImGui::GetColorU32(ProjectHub::Internal::cardBackground), 8.0f);
            const char* title = "No recent projects";
            const char* description = "Open an existing project or create a new one.";
            ImGui::GetWindowDrawList()->AddText(ImVec2(emptyMin.x + 16.0f, emptyMin.y + 51.0f), ImGui::GetColorU32(ProjectHub::Internal::accent), Icons::Tabler::folderOff);
            ImGui::GetWindowDrawList()->AddText(ImVec2(emptyMin.x + 46.0f, emptyMin.y + 31.0f), ImGui::GetColorU32(ImGuiCol_Text), title);
            ImGui::GetWindowDrawList()->AddText(ImVec2(emptyMin.x + 46.0f, emptyMin.y + 59.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), description);
            ImGui::Dummy(ImVec2(0.0f, 118.0f));
        }
        ImGui::EndChild();

        if (!projectToOpen.empty()) {
            OpenProject(inWindow, projectToOpen, inRhiType);
        }
    }

    void ProjectHubFrame::RenderCreateProjectPopup(EditorWindow& inWindow, const std::string& inRhiType)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(456.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.075f, 0.086f, 0.105f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 22.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 11.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::BeginPopupModal("##CreateProjectPopup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            const std::string createProjectLabel = Widgets::Label(Icons::Tabler::templateIcon, "Create project");
            ImGui::TextUnformatted(createProjectLabel.c_str());
            ImGui::TextDisabled("Start from a template and open it right away.");
            ImGui::Dummy(ImVec2(0.0f, 13.0f));

            ImGui::TextDisabled("NAME");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint("##ProjectName", "MyProject", &projectName);
            ImGui::Dummy(ImVec2(0.0f, 7.0f));

            ImGui::TextDisabled("LOCATION");
            const float browseButtonWidth = 76.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseButtonWidth - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint("##ProjectDirectory", "Choose a parent folder", &directory);
            ImGui::SameLine();
            const std::string browseLabel = Widgets::Label(Icons::Tabler::folderOpen, "Browse");
            if (ImGui::Button(browseLabel.c_str(), ImVec2(browseButtonWidth, 0.0f))) {
                if (const auto selectedDirectory = PlatformUtils::SelectDirectory("Choose Project Location", directory)) {
                    directory = *selectedDirectory;
                }
            }
            ImGui::Dummy(ImVec2(0.0f, 7.0f));

            ImGui::TextDisabled("TEMPLATE");
            const char* selectedTemplate = projectTemplates.empty()
                ? "No templates available"
                : projectTemplates[selectedTemplateIndex].name.c_str();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##ProjectTemplate", selectedTemplate)) {
                for (int i = 0; i < static_cast<int>(projectTemplates.size()); i++) {
                    const bool selected = i == selectedTemplateIndex;
                    const std::string templateLabel = Widgets::Label(Icons::Tabler::templateIcon, projectTemplates[i].name);
                    if (ImGui::Selectable(templateLabel.c_str(), selected)) {
                        selectedTemplateIndex = i;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (!statusMessage.empty()) {
                ImGui::Dummy(ImVec2(0.0f, 9.0f));
                const std::string statusLabel = Widgets::Label(Icons::Tabler::circleX, statusMessage);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.42f, 0.42f, 1.0f));
                ImGui::TextWrapped("%s", statusLabel.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Dummy(ImVec2(0.0f, 15.0f));
            const float footerWidth = 178.0f;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - footerWidth);
            const std::string cancelLabel = Widgets::Label(Icons::Tabler::x, "Cancel");
            if (ImGui::Button(cancelLabel.c_str(), ImVec2(82.0f, 36.0f))) {
                statusMessage.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            const std::string confirmLabel = Widgets::Label(Icons::Tabler::check, "Create##Confirm");
            if (ProjectHub::Internal::RenderActionButton(confirmLabel.c_str(), ImVec2(88.0f, 36.0f), ProjectHub::Internal::accent, ProjectHub::Internal::accentHovered)) {
                const CreateProjectResult result = CreateProject();
                if (result.success) {
                    statusMessage.clear();
                    ImGui::CloseCurrentPopup();
                    OpenProject(inWindow, result.projectPath, inRhiType);
                } else {
                    statusMessage = result.error;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor();
    }

    CreateProjectResult ProjectHubFrame::CreateProject()
    {
        if (projectName.empty() || directory.empty() || projectTemplates.empty()
            || selectedTemplateIndex < 0 || selectedTemplateIndex >= static_cast<int>(projectTemplates.size())) {
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

        if (const auto result = ProjectHub::Internal::RenderProjectTemplate(templateDir, projectDir, projectName);
            result.IsErr()) {
            return { .success = false, .error = result.Error(), .projectPath = {} };
        }

        TouchRecentProject(projectDir.String());
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

        const std::string command = ProjectHub::Internal::LaunchCommand(Core::Paths::ExecutablePath().String(), inProjectPath, inRhiType);
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

//
// Created by johnk on 2026/7/8.
//

#pragma once

#include <string>
#include <vector>

#include <Common/FileSystem.h>
#include <Mirror/Meta.h>

namespace Editor {
    struct EClass() RecentProjectInfo {
        EClassBody(RecentProjectInfo)

        EProperty() std::string name;
        EProperty() std::string path;
    };

    struct EClass() ProjectTemplateInfo {
        EClassBody(ProjectTemplateInfo)

        EProperty() std::string name;
        EProperty() std::string path;
    };

    struct EClass() CreateProjectResult {
        EClassBody(CreateProjectResult)

        EProperty() bool success;
        EProperty() std::string error;
        EProperty() std::string projectPath;
    };

    class EditorWindow;

    class ProjectHubFrame final {
    public:
        ProjectHubFrame();
        ~ProjectHubFrame();

        void Render(EditorWindow& inWindow, const std::string& inRhiType);

    private:
        void RenderActionBar(EditorWindow& inWindow, const std::string& inRhiType);
        void RenderRecentProjects(EditorWindow& inWindow, const std::string& inRhiType);
        void RenderCreateProjectPopup(EditorWindow& inWindow, const std::string& inRhiType);
        CreateProjectResult CreateProject();
        void OpenProject(EditorWindow& inWindow, const std::string& inProjectPath, const std::string& inRhiType);
        void SaveRecentProjects() const;
        void TouchRecentProject(const std::string& inProjectPath);

        Common::Path recentProjectsFile;
        std::vector<ProjectTemplateInfo> projectTemplates;
        std::vector<RecentProjectInfo> recentProjects;
        std::string projectName;
        std::string directory;
        int selectedTemplateIndex;
        std::string statusMessage;
    };
}

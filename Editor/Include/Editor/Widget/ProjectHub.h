//
// Created by johnk on 2025/8/3.
//

#pragma once

#include <QJsonValue>

#include <Editor/Widget/WebWidget.h>
#include <Mirror/Meta.h>
#include <Common/FileSystem.h>
#include <Editor/Qt/EngineSerialization.h>

namespace Editor {
    class ProjectHub;

    struct EClass() RecentProjectInfo {
        EClassBody(RecentProjectInfo)

        EProperty() QString name;
        EProperty() QString path;
    };

    class ProjectHubBackend final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString engineVersion MEMBER engineVersion CONSTANT)
        Q_PROPERTY(QJsonValue recentProjects READ GetRecentProjects)

    public:
        explicit ProjectHubBackend(ProjectHub* parent = nullptr);
        ~ProjectHubBackend() override;

    public Q_SLOTS:
        void CreateProject() const;

    private:
        QJsonValue GetRecentProjects() const;

        Common::Path recentProjectsFile;
        QString engineVersion;
        std::vector<RecentProjectInfo> recentProjects;
    };

    class ProjectHub final : public WebWidget {
        Q_OBJECT

    public:
        explicit ProjectHub(QWidget* inParent = nullptr);

    private:
        ProjectHubBackend* backend;
    };
}

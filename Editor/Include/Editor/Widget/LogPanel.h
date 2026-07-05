//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <Editor/EditorLog.h>
#include <Editor/Widget/WebWidget.h>
#include <Mirror/Meta.h>

namespace Editor {
    struct EClass() LogEntryInfo {
        EClassBody(LogEntryInfo)

        EProperty() std::string time;
        EProperty() std::string tag;
        EProperty() std::string level;
        EProperty() std::string content;
    };

    class LogBackend final : public QObject {
        Q_OBJECT

    public:
        explicit LogBackend(QObject* inParent = nullptr);
        ~LogBackend() override;

    public Q_SLOTS:
        QJsonValue GetHistory() const;

    Q_SIGNALS:
        void EntryAppended(QJsonValue inEntry);

    private:
        EditorLogStream::ListenerHandle listenerHandle;
    };

    class LogPanel final : public WebWidget {
        Q_OBJECT

    public:
        explicit LogPanel(QWidget* inParent = nullptr);

    private:
        LogBackend* backend;
    };
}

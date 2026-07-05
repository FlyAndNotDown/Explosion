//
// Created by johnk on 2026/7/5.
//

#include <Editor/Qt/JsonSerialization.h>
#include <Editor/Widget/LogPanel.h>
#include <Editor/Widget/moc_LogPanel.cpp> // NOLINT

namespace Editor::Internal {
    static LogEntryInfo ToLogEntryInfo(const Core::LogEntry& inEntry)
    {
        static std::unordered_map<Core::LogLevel, std::string> levelStringMap = {
            { Core::LogLevel::verbose, "verbose" },
            { Core::LogLevel::info, "info" },
            { Core::LogLevel::warning, "warning" },
            { Core::LogLevel::error, "error" }
        };

        LogEntryInfo result;
        result.time = inEntry.time;
        result.tag = inEntry.tag;
        result.level = levelStringMap.at(inEntry.level);
        result.content = inEntry.content;
        return result;
    }
}

namespace Editor {
    LogBackend::LogBackend(QObject* inParent)
        : QObject(inParent)
    {
        // entries arrive from arbitrary threads, marshal them onto the widget thread before emitting to the web page
        listenerHandle = EditorLogStream::Get().AddListener([this](const Core::LogEntry& inEntry) -> void {
            QMetaObject::invokeMethod(this, [this, entry = inEntry]() -> void {
                QJsonValue json;
                QtJsonSerialize(json, Internal::ToLogEntryInfo(entry));
                emit EntryAppended(json);
            }, Qt::QueuedConnection);
        });
    }

    LogBackend::~LogBackend()
    {
        EditorLogStream::Get().RemoveListener(listenerHandle);
    }

    QJsonValue LogBackend::GetHistory() const
    {
        std::vector<LogEntryInfo> entries;
        for (const auto& entry : EditorLogStream::Get().Snapshot()) {
            entries.emplace_back(Internal::ToLogEntryInfo(entry));
        }

        QJsonValue result;
        QtJsonSerialize(result, entries);
        return result;
    }

    LogPanel::LogPanel(QWidget* inParent)
        : WebWidget(inParent)
    {
        Load("/editor/log");
        backend = new LogBackend(this);
        GetWebChannel()->registerObject("backend", backend);
    }
}

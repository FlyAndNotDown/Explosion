//
// Created by johnk on 2026/7/5.
//

#include <ranges>

#include <Editor/EditorLog.h>

namespace Editor {
    EditorLogStream& EditorLogStream::Get()
    {
        static EditorLogStream* stream = []() -> EditorLogStream* {
            auto* result = new EditorLogStream();
            Core::Logger::Get().Attach(Common::UniquePtr<Core::LogStream>(result));
            return result;
        }();
        return *stream;
    }

    EditorLogStream::EditorLogStream()
        : nextListenerHandle(0)
    {
    }

    void EditorLogStream::Write(const Core::LogEntry& inEntry)
    {
        std::unique_lock lock(mutex);
        history.emplace_back(inEntry);
        if (history.size() > maxHistoryEntries) {
            history.pop_front();
        }
        for (const auto& listener : listeners | std::views::values) {
            listener(inEntry);
        }
    }

    void EditorLogStream::Flush()
    {
    }

    std::vector<Core::LogEntry> EditorLogStream::Snapshot() const
    {
        std::unique_lock lock(mutex);
        return { history.begin(), history.end() };
    }

    EditorLogStream::ListenerHandle EditorLogStream::AddListener(EntryListener inListener)
    {
        std::unique_lock lock(mutex);
        const ListenerHandle handle = nextListenerHandle++;
        listeners.emplace(handle, std::move(inListener));
        return handle;
    }

    void EditorLogStream::RemoveListener(ListenerHandle inHandle)
    {
        std::unique_lock lock(mutex);
        listeners.erase(inHandle);
    }
}

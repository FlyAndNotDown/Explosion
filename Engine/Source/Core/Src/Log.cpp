//
// Created by johnk on 2025/1/13.
//

#include <Core/Log.h>
#include <Common/FileSystem.h>
#include <Common/IO.h>

namespace Core::Internal {
    static std::string FormatLogEntry(const LogEntry& inEntry)
    {
        static std::unordered_map<LogLevel, std::string_view> logLevelStringMap = {
            { LogLevel::verbose, "Verbose" },
            { LogLevel::info, "Info" },
            { LogLevel::warning, "Warning" },
            { LogLevel::error, "Error" }
        };

        static std::unordered_map<LogLevel, std::string_view> logLevelColorStr = {
            { LogLevel::verbose, "" },
            { LogLevel::info, "" },
            { LogLevel::warning, "\033[33m" },
            { LogLevel::error, "\033[31m" }
        };

        return std::format("{}[{}][{}][{}] {}\033[0m", logLevelColorStr.at(inEntry.level), inEntry.time, inEntry.tag, logLevelStringMap.at(inEntry.level), inEntry.content);
    }
}

namespace Core {
    void LogStream::Write(const LogEntry& inEntry)
    {
        Write(Internal::FormatLogEntry(inEntry));
    }

    COutLogStream::COutLogStream() = default;

    COutLogStream::~COutLogStream()
    {
        Flush();
    }

    void COutLogStream::Write(const std::string& inString)
    {
        std::cout << inString << Common::newline;
    }

    void COutLogStream::Flush()
    {
        std::cout << std::flush;
    }

    FileLogStream::FileLogStream(const std::string& inFilePath)
    {
        if (const auto parentPath = Common::Path(inFilePath).Parent();
            !parentPath.Exists()) {
            parentPath.MakeDir();
        }
        file = std::ofstream(inFilePath);
    }

    FileLogStream::~FileLogStream()
    {
        Flush();
    }

    void FileLogStream::Write(const std::string& inString)
    {
        file << inString << Common::newline;
    }

    void FileLogStream::Flush()
    {
        file << std::flush;
    }

    Logger& Logger::Get()
    {
        static Logger logger;
        return logger;
    }

    Logger::~Logger()
    {
        Flush();
    }

    void Logger::Log(const std::string& inTag, LogLevel inLevel, const std::string& inContent)
    {
        LogEntry entry;
        entry.time = Common::AccurateTime(Common::TimePoint::Now()).ToString("hh-mm-ss:mss");
        entry.tag = inTag;
        entry.level = inLevel;
        entry.content = inContent;
        LogInternal(entry);
    }

    void Logger::Attach(Common::UniquePtr<LogStream>&& inStream)
    {
        streams.emplace_back(std::move(inStream));
    }

    void Logger::Flush() // NOLINT
    {
        for (const auto& stream : streams) {
            stream->Flush();
        }
    }

    Logger::Logger()
        : lastFlushTimeSec(Common::TimePoint::Now().ToSeconds())
    {
        Attach(new COutLogStream());
    }

    void Logger::LogInternal(const LogEntry& inEntry) // NOLINT
    {
#if BUILD_CONFIG_DEBUG
        const bool needFlush = true; // NOLINT
#else
        const auto timeNowSec = Common::TimePoint::Now().ToSeconds();
        const bool needFlush = timeNowSec - lastFlushTimeSec > 5.0f;
        lastFlushTimeSec = timeNowSec;
#endif

        for (const auto& stream : streams) {
            stream->Write(inEntry);
            if (needFlush) {
                stream->Flush();
            }
        }
    }
}

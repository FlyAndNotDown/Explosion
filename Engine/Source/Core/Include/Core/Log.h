//
// Created by johnk on 2025/1/13.
//

#pragma once

#include <iostream>
#include <fstream>
#include <format>

#include <Common/Memory.h>
#include <Common/Time.h>
#include <Common/Utility.h>
#include <Core/Api.h>

#define LogVerbose(tag, ...) Core::Logger::Get().Log(#tag, Core::LogLevel::verbose, std::format(__VA_ARGS__))
#define LogInfo(tag, ...) Core::Logger::Get().Log(#tag, Core::LogLevel::info, std::format(__VA_ARGS__))
#define LogWarning(tag, ...) Core::Logger::Get().Log(#tag, Core::LogLevel::warning, std::format(__VA_ARGS__))
#define LogError(tag, ...) Core::Logger::Get().Log(#tag, Core::LogLevel::error, std::format(__VA_ARGS__))

namespace Core {
    enum class LogLevel : uint8_t {
        verbose,
        info,
        warning,
        error,
        max
    };

    struct LogEntry {
        std::string time;
        std::string tag;
        LogLevel level;
        std::string content;
    };

    class CORE_API LogStream {
    public:
        virtual ~LogStream() = default;
        virtual void Write(const std::string& inString) = 0;
        // structured entry point used by Logger, the default implementation formats the entry and forwards to
        // Write(string), override it when the stream wants the raw fields (e.g. the editor log panel)
        virtual void Write(const LogEntry& inEntry);
        virtual void Flush() = 0;
    };

    class CORE_API COutLogStream final : public LogStream {
    public:
        COutLogStream();
        ~COutLogStream() override;

        NonCopyable(COutLogStream);
        NonMovable(COutLogStream);

        void Write(const std::string& inString) override;
        void Flush() override;
    };

    class CORE_API FileLogStream final : public LogStream {
    public:
        explicit FileLogStream(const std::string& inFilePath);
        ~FileLogStream() override;

        NonCopyable(FileLogStream)
        NonMovable(FileLogStream)

        void Write(const std::string& inString) override;
        void Flush() override;

    private:
        std::ofstream file;
    };

    class CORE_API Logger {
    public:
        static Logger& Get();

        ~Logger();
        NonCopyable(Logger)
        NonMovable(Logger)

        void Log(const std::string& inTag, LogLevel inLevel, const std::string& inContent);
        void Attach(Common::UniquePtr<LogStream>&& inStream);
        void Flush();

    private:
        Logger();

        void LogInternal(const LogEntry& inEntry);

        float lastFlushTimeSec;
        std::vector<Common::UniquePtr<LogStream>> streams;
    };
}

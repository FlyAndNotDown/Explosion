#include <Common/Process.h>

#if PLATFORM_WINDOWS
#include <windows.h>

#include <Common/String.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>

#if PLATFORM_MACOS
#include <crt_externs.h>
#else
extern char** environ;
#endif
#endif

namespace Common::Internal {
#if PLATFORM_WINDOWS
    static void AppendWindowsCommandLineArgument(std::wstring& outCommandLine, const std::wstring& inArgument)
    {
        if (!outCommandLine.empty()) {
            outCommandLine.push_back(L' ');
        }

        outCommandLine.push_back(L'"');
        size_t backslashCount = 0;
        for (const wchar_t character : inArgument) {
            if (character == L'\\') {
                ++backslashCount;
                continue;
            }
            if (character == L'"') {
                outCommandLine.append(backslashCount * 2 + 1, L'\\');
            } else {
                outCommandLine.append(backslashCount, L'\\');
            }
            backslashCount = 0;
            outCommandLine.push_back(character);
        }
        outCommandLine.append(backslashCount * 2, L'\\');
        outCommandLine.push_back(L'"');
    }
#else
    static char** GetEnvironment()
    {
#if PLATFORM_MACOS
        return *_NSGetEnviron();
#else
        return environ;
#endif
    }

    static std::optional<int32_t> WaitForProcess(const pid_t processId)
    {
        int status = 0;
        while (waitpid(processId, &status, 0) == -1) {
            if (errno != EINTR) {
                return std::nullopt;
            }
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return std::nullopt;
    }
#endif
}

namespace Common {
    std::optional<int32_t> Process::Run(const std::string& inExecutablePath, const std::vector<std::string>& inArguments)
    {
#if PLATFORM_WINDOWS
        const std::wstring executablePath = StringUtils::ToWideString(inExecutablePath);
        std::wstring commandLine;
        Internal::AppendWindowsCommandLineArgument(commandLine, executablePath);
        for (const auto& argument : inArguments) {
            Internal::AppendWindowsCommandLineArgument(commandLine, StringUtils::ToWideString(argument));
        }

        STARTUPINFOW startupInfo {};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo {};
        if (!CreateProcessW(executablePath.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo)) {
            return std::nullopt;
        }

        const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD exitCode = 0;
        const bool hasExitCode = waitResult == WAIT_OBJECT_0 && GetExitCodeProcess(processInfo.hProcess, &exitCode);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return hasExitCode ? std::optional<int32_t>(static_cast<int32_t>(exitCode)) : std::nullopt;
#else
        std::vector<char*> arguments;
        arguments.reserve(inArguments.size() + 2);
        arguments.push_back(const_cast<char*>(inExecutablePath.c_str()));
        for (const auto& argument : inArguments) {
            arguments.push_back(const_cast<char*>(argument.c_str()));
        }
        arguments.push_back(nullptr);

        pid_t processId = 0;
        if (posix_spawn(&processId, inExecutablePath.c_str(), nullptr, nullptr, arguments.data(), Internal::GetEnvironment()) != 0) {
            return std::nullopt;
        }
        return Internal::WaitForProcess(processId);
#endif
    }
}

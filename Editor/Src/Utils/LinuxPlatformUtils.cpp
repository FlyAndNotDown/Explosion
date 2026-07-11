//
// Created by johnk on 2026/7/11.
//

#include <Editor/Utils/PlatformUtils.h>

#if PLATFORM_LINUX

#include <array>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace Editor::Internal {
    static std::string QuoteShellArgument(const std::string& inArgument)
    {
        std::string result = "'";
        for (const char character : inArgument) {
            if (character == '\'') {
                result += "'\\''";
            } else {
                result += character;
            }
        }
        result += "'";
        return result;
    }

    static bool HasCommand(const char* inCommand)
    {
        return std::system((std::string("command -v ") + inCommand + " >/dev/null 2>&1").c_str()) == 0;
    }

    static std::optional<std::string> RunDirectoryDialog(const std::string& inCommand)
    {
        std::FILE* const process = popen(inCommand.c_str(), "r");
        if (process == nullptr) {
            return std::nullopt;
        }

        std::array<char, 1024> buffer {};
        std::string result;
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), process) != nullptr) {
            result += buffer.data();
        }
        pclose(process);

        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result.empty() ? std::nullopt : std::optional<std::string>(std::move(result));
    }
}

namespace Editor {
    std::optional<std::string> PlatformUtils::SelectDirectory(const std::string& inTitle, const std::string& inInitialDirectory)
    {
        if (Internal::HasCommand("zenity")) {
            const std::string initialPath = inInitialDirectory.empty() ? std::string() : inInitialDirectory + "/";
            return Internal::RunDirectoryDialog("zenity --file-selection --directory --title=" + Internal::QuoteShellArgument(inTitle) + " --filename=" + Internal::QuoteShellArgument(initialPath) + " 2>/dev/null");
        }
        if (Internal::HasCommand("kdialog")) {
            return Internal::RunDirectoryDialog("kdialog --getexistingdirectory " + Internal::QuoteShellArgument(inInitialDirectory) + " --title " + Internal::QuoteShellArgument(inTitle) + " 2>/dev/null");
        }
        return std::nullopt;
    }
}

#endif

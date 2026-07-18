//
// Created by johnk on 2026/7/18.
//

#include <algorithm>
#include <cctype>
#include <format>
#include <system_error>

#include <Editor/Asset/AssetFileSystem.h>

namespace Editor::Internal {
    static bool IsValidEntryName(const std::string& inName)
    {
        if (inName.empty() || inName == "." || inName == "..") {
            return false;
        }
        const bool containsControlCharacter = std::ranges::any_of(inName, [](unsigned char inCharacter) -> bool {
            return inCharacter < 32;
        });
        return !containsControlCharacter
            && inName.find_first_of("/\\<>:\"|?*") == std::string::npos
            && inName.back() != ' '
            && inName.back() != '.';
    }

    static bool IsPathPrefix(const std::filesystem::path& inPrefix, const std::filesystem::path& inPath)
    {
        auto prefixIter = inPrefix.begin();
        auto pathIter = inPath.begin();
        for (; prefixIter != inPrefix.end() && pathIter != inPath.end(); ++prefixIter, ++pathIter) {
            if (*prefixIter != *pathIter) {
                return false;
            }
        }
        return prefixIter == inPrefix.end();
    }

    static std::string ErrorMessage(const std::string& inOperation, const std::error_code& inError)
    {
        return std::format("{}: {}", inOperation, inError.message());
    }

    static std::string Lowercase(std::string inValue)
    {
        std::ranges::transform(inValue, inValue.begin(), [](unsigned char inCharacter) -> char {
            return static_cast<char>(std::tolower(inCharacter));
        });
        return inValue;
    }
}

namespace Editor {
    AssetFileSystem::AssetFileSystem(std::filesystem::path inRoot)
    {
        std::error_code error;
        std::filesystem::create_directories(inRoot, error);
        error.clear();
        const std::filesystem::path absoluteRoot = std::filesystem::absolute(inRoot, error);
        if (error) {
            root = inRoot.lexically_normal();
            return;
        }
        root = std::filesystem::weakly_canonical(absoluteRoot, error);
        if (error) {
            root = absoluteRoot.lexically_normal();
        }
    }

    const std::filesystem::path& AssetFileSystem::Root() const
    {
        return root;
    }

    bool AssetFileSystem::Contains(const std::filesystem::path& inPath) const
    {
        std::error_code error;
        const std::filesystem::path normalized = std::filesystem::weakly_canonical(std::filesystem::absolute(inPath), error);
        return !error && Internal::IsPathPrefix(root, normalized);
    }

    std::vector<AssetFileEntry> AssetFileSystem::List(const std::filesystem::path& inDirectory, std::string& outError) const
    {
        outError.clear();
        if (!ValidateDirectory(inDirectory, outError)) {
            return {};
        }

        std::vector<AssetFileEntry> result;
        std::error_code error;
        std::filesystem::directory_iterator iter(inDirectory, std::filesystem::directory_options::skip_permission_denied, error);
        const std::filesystem::directory_iterator end;
        while (!error && iter != end) {
            const std::filesystem::directory_entry& entry = *iter;
            std::error_code entryError;
            if (!entry.is_symlink(entryError)) {
                const bool isDirectory = entry.is_directory(entryError);
                const uintmax_t size = !entryError && !isDirectory ? entry.file_size(entryError) : 0;
                if (!entryError) {
                    result.emplace_back(AssetFileEntry {.path = entry.path(), .directory = isDirectory, .size = size});
                }
            }
            iter.increment(error);
        }
        if (error) {
            outError = Internal::ErrorMessage("Could not list assets", error);
        }

        std::ranges::sort(result, [](const AssetFileEntry& inLeft, const AssetFileEntry& inRight) -> bool {
            if (inLeft.directory != inRight.directory) {
                return inLeft.directory;
            }
            return Internal::Lowercase(inLeft.path.filename().string()) < Internal::Lowercase(inRight.path.filename().string());
        });
        return result;
    }

    bool AssetFileSystem::CreateFolder(const std::filesystem::path& inParent, const std::string& inName, std::filesystem::path& outCreated, std::string& outError) const
    {
        outError.clear();
        if (!ValidateDirectory(inParent, outError)) {
            return false;
        }
        if (!Internal::IsValidEntryName(inName)) {
            outError = "Folder names cannot be empty or contain reserved path characters";
            return false;
        }

        outCreated = inParent / inName;
        std::error_code error;
        if (std::filesystem::exists(outCreated, error)) {
            outError = "An item with that name already exists";
            return false;
        }
        if (!std::filesystem::create_directory(outCreated, error)) {
            outError = error ? Internal::ErrorMessage("Could not create folder", error) : "Could not create folder";
            return false;
        }
        return true;
    }

    bool AssetFileSystem::Rename(const std::filesystem::path& inSource, const std::string& inName, std::filesystem::path& outRenamed, std::string& outError) const
    {
        outError.clear();
        if (!ValidateManagedPath(inSource, false, outError)) {
            return false;
        }
        if (!Internal::IsValidEntryName(inName)) {
            outError = "Names cannot be empty or contain reserved path characters";
            return false;
        }

        outRenamed = inSource.parent_path() / inName;
        if (outRenamed == inSource) {
            return true;
        }

        std::error_code error;
        if (std::filesystem::exists(outRenamed, error)) {
            outError = "An item with that name already exists";
            return false;
        }
        std::filesystem::rename(inSource, outRenamed, error);
        if (error) {
            outError = Internal::ErrorMessage("Could not rename asset", error);
            return false;
        }
        return true;
    }

    bool AssetFileSystem::Remove(const std::filesystem::path& inSource, std::string& outError) const
    {
        outError.clear();
        if (!ValidateManagedPath(inSource, false, outError)) {
            return false;
        }

        std::error_code error;
        std::filesystem::remove_all(inSource, error);
        if (error) {
            outError = Internal::ErrorMessage("Could not delete asset", error);
            return false;
        }
        return true;
    }

    bool AssetFileSystem::Transfer(const std::filesystem::path& inSource, const std::filesystem::path& inDestinationDirectory, AssetFileTransferMode inMode, std::filesystem::path& outDestination, std::string& outError) const
    {
        outError.clear();
        if (!ValidateManagedPath(inSource, false, outError) || !ValidateDirectory(inDestinationDirectory, outError)) {
            return false;
        }
        if (inMode == AssetFileTransferMode::move && inSource.parent_path() == inDestinationDirectory) {
            outError = "The asset is already in this folder";
            return false;
        }

        std::error_code error;
        const bool sourceIsDirectory = std::filesystem::is_directory(inSource, error);
        if (error) {
            outError = Internal::ErrorMessage("Could not inspect asset", error);
            return false;
        }
        if (sourceIsDirectory) {
            const std::filesystem::path normalizedSource = std::filesystem::weakly_canonical(inSource, error);
            const std::filesystem::path normalizedDestination = std::filesystem::weakly_canonical(inDestinationDirectory, error);
            if (error) {
                outError = Internal::ErrorMessage("Could not inspect destination", error);
                return false;
            }
            if (Internal::IsPathPrefix(normalizedSource, normalizedDestination)) {
                outError = "A folder cannot be placed inside itself";
                return false;
            }
        }

        outDestination = AvailableDestination(inDestinationDirectory, inSource.filename());
        if (inMode == AssetFileTransferMode::copy) {
            const auto options = sourceIsDirectory
                ? std::filesystem::copy_options::recursive
                : std::filesystem::copy_options::none;
            std::filesystem::copy(inSource, outDestination, options, error);
        } else {
            std::filesystem::rename(inSource, outDestination, error);
        }
        if (error) {
            if (inMode == AssetFileTransferMode::copy) {
                std::error_code cleanupError;
                std::filesystem::remove_all(outDestination, cleanupError);
            }
            outError = Internal::ErrorMessage(inMode == AssetFileTransferMode::copy ? "Could not copy asset" : "Could not move asset", error);
            return false;
        }
        return true;
    }

    bool AssetFileSystem::Import(const std::filesystem::path& inSource, const std::filesystem::path& inDestinationDirectory, std::filesystem::path& outDestination, std::string& outError) const
    {
        outError.clear();
        if (!ValidateDirectory(inDestinationDirectory, outError)) {
            return false;
        }

        std::error_code error;
        if (!std::filesystem::is_regular_file(inSource, error)) {
            outError = error ? Internal::ErrorMessage("Could not inspect import", error) : "Only files can be imported";
            return false;
        }
        outDestination = AvailableDestination(inDestinationDirectory, inSource.filename());
        std::filesystem::copy_file(inSource, outDestination, std::filesystem::copy_options::none, error);
        if (error) {
            std::error_code cleanupError;
            std::filesystem::remove(outDestination, cleanupError);
            outError = Internal::ErrorMessage("Could not import asset", error);
            return false;
        }
        return true;
    }

    bool AssetFileSystem::ValidateManagedPath(const std::filesystem::path& inPath, bool inAllowRoot, std::string& outError) const
    {
        std::error_code error;
        if (!std::filesystem::exists(inPath, error)) {
            outError = error ? Internal::ErrorMessage("Could not inspect asset", error) : "The asset no longer exists";
            return false;
        }
        if (!Contains(inPath)) {
            outError = "The requested path is outside the project asset directory";
            return false;
        }
        if (!inAllowRoot) {
            const bool isRoot = std::filesystem::equivalent(inPath, root, error);
            if (error) {
                outError = Internal::ErrorMessage("Could not inspect asset", error);
                return false;
            }
            if (isRoot) {
                outError = "The project asset directory cannot be modified";
                return false;
            }
        }
        const bool isSymlink = std::filesystem::is_symlink(inPath, error);
        if (error) {
            outError = Internal::ErrorMessage("Could not inspect asset", error);
            return false;
        }
        if (isSymlink) {
            outError = "Symbolic links are not managed by the asset browser";
            return false;
        }
        return true;
    }

    bool AssetFileSystem::ValidateDirectory(const std::filesystem::path& inPath, std::string& outError) const
    {
        if (!ValidateManagedPath(inPath, true, outError)) {
            return false;
        }
        std::error_code error;
        if (!std::filesystem::is_directory(inPath, error)) {
            outError = error ? Internal::ErrorMessage("Could not inspect folder", error) : "The destination is not a folder";
            return false;
        }
        return true;
    }

    std::filesystem::path AssetFileSystem::AvailableDestination(const std::filesystem::path& inDirectory, const std::filesystem::path& inFileName) const
    {
        std::filesystem::path result = inDirectory / inFileName;
        if (!std::filesystem::exists(result)) {
            return result;
        }

        const std::string stem = inFileName.stem().string();
        const std::string extension = inFileName.extension().string();
        for (size_t index = 1;; index++) {
            const std::string suffix = index == 1 ? " Copy" : std::format(" Copy {}", index);
            result = inDirectory / (stem + suffix + extension);
            if (!std::filesystem::exists(result)) {
                return result;
            }
        }
    }
}

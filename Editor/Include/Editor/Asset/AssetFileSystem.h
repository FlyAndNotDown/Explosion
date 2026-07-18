//
// Created by johnk on 2026/7/18.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Editor {
    enum class AssetFileTransferMode : uint8_t {
        copy,
        move
    };

    struct AssetFileEntry {
        std::filesystem::path path;
        bool directory;
        uintmax_t size;
    };

    class AssetFileSystem final {
    public:
        explicit AssetFileSystem(std::filesystem::path inRoot);

        const std::filesystem::path& Root() const;
        bool Contains(const std::filesystem::path& inPath) const;
        std::vector<AssetFileEntry> List(const std::filesystem::path& inDirectory, std::string& outError) const;
        bool CreateFolder(const std::filesystem::path& inParent, const std::string& inName, std::filesystem::path& outCreated, std::string& outError) const;
        bool Rename(const std::filesystem::path& inSource, const std::string& inName, std::filesystem::path& outRenamed, std::string& outError) const;
        bool Remove(const std::filesystem::path& inSource, std::string& outError) const;
        bool Transfer(const std::filesystem::path& inSource, const std::filesystem::path& inDestinationDirectory, AssetFileTransferMode inMode, std::filesystem::path& outDestination, std::string& outError) const;
        bool Import(const std::filesystem::path& inSource, const std::filesystem::path& inDestinationDirectory, std::filesystem::path& outDestination, std::string& outError) const;

    private:
        bool ValidateManagedPath(const std::filesystem::path& inPath, bool inAllowRoot, std::string& outError) const;
        bool ValidateDirectory(const std::filesystem::path& inPath, std::string& outError) const;
        std::filesystem::path AvailableDestination(const std::filesystem::path& inDirectory, const std::filesystem::path& inFileName) const;

        std::filesystem::path root;
    };
}

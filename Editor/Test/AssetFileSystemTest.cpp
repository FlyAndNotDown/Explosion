//
// Created by johnk on 2026/7/18.
//

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>

#include <Editor/Asset/AssetFileSystem.h>
#include <Test/Test.h>

namespace Editor::Test {
    class AssetFileSystemTest : public testing::Test {
    protected:
        void SetUp() override
        {
            const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
            testDirectory = std::filesystem::temp_directory_path() / std::format("ExplosionAssetFileSystemTest-{}", uniqueId);
            assetDirectory = testDirectory / "Asset";
            externalDirectory = testDirectory / "External";
            std::filesystem::create_directories(externalDirectory);
        }

        void TearDown() override
        {
            std::error_code error;
            std::filesystem::remove_all(testDirectory, error);
        }

        static void WriteFile(const std::filesystem::path& inPath, const std::string& inContent)
        {
            std::ofstream stream(inPath, std::ios::binary);
            stream << inContent;
        }

        std::filesystem::path testDirectory;
        std::filesystem::path assetDirectory;
        std::filesystem::path externalDirectory;
    };

    TEST_F(AssetFileSystemTest, CreatesListsAndRenamesFolders)
    {
        AssetFileSystem fileSystem(assetDirectory);
        std::filesystem::path created;
        std::string error;

        ASSERT_TRUE(fileSystem.CreateFolder(fileSystem.Root(), "Materials", created, error)) << error;
        EXPECT_TRUE(fileSystem.Contains(created));
        ASSERT_FALSE(fileSystem.CreateFolder(fileSystem.Root(), "Materials", created, error));
        EXPECT_FALSE(error.empty());

        const std::vector<AssetFileEntry> entries = fileSystem.List(fileSystem.Root(), error);
        ASSERT_TRUE(error.empty()) << error;
        ASSERT_EQ(entries.size(), 1);
        EXPECT_TRUE(entries.front().directory);
        EXPECT_EQ(entries.front().path.filename(), "Materials");

        std::filesystem::path renamed;
        ASSERT_TRUE(fileSystem.Rename(entries.front().path, "Textures", renamed, error)) << error;
        EXPECT_TRUE(std::filesystem::is_directory(renamed));
        EXPECT_FALSE(std::filesystem::exists(created));
    }

    TEST_F(AssetFileSystemTest, CopiesMovesAndRemovesAssets)
    {
        AssetFileSystem fileSystem(assetDirectory);
        std::filesystem::path folder;
        std::string error;
        ASSERT_TRUE(fileSystem.CreateFolder(fileSystem.Root(), "Meshes", folder, error)) << error;

        const std::filesystem::path asset = fileSystem.Root() / "Cube.expa";
        WriteFile(asset, "cube");

        std::filesystem::path copied;
        ASSERT_TRUE(fileSystem.Transfer(asset, fileSystem.Root(), AssetFileTransferMode::copy, copied, error)) << error;
        EXPECT_EQ(copied.filename(), "Cube Copy.expa");
        EXPECT_TRUE(std::filesystem::exists(asset));

        std::filesystem::path moved;
        ASSERT_TRUE(fileSystem.Transfer(asset, folder, AssetFileTransferMode::move, moved, error)) << error;
        EXPECT_EQ(moved, folder / "Cube.expa");
        EXPECT_FALSE(std::filesystem::exists(asset));

        std::filesystem::path copiedFolder;
        ASSERT_TRUE(fileSystem.Transfer(folder, fileSystem.Root(), AssetFileTransferMode::copy, copiedFolder, error)) << error;
        EXPECT_EQ(copiedFolder.filename(), "Meshes Copy");
        EXPECT_TRUE(std::filesystem::exists(copiedFolder / "Cube.expa"));

        ASSERT_TRUE(fileSystem.Remove(copied, error)) << error;
        EXPECT_FALSE(std::filesystem::exists(copied));
    }

    TEST_F(AssetFileSystemTest, ImportsWithoutOverwritingAndRejectsEscapes)
    {
        AssetFileSystem fileSystem(assetDirectory);
        const std::filesystem::path externalAsset = externalDirectory / "Texture.expa";
        WriteFile(externalAsset, "texture");

        std::filesystem::path imported;
        std::string error;
        ASSERT_TRUE(fileSystem.Import(externalAsset, fileSystem.Root(), imported, error)) << error;
        ASSERT_TRUE(fileSystem.Import(externalAsset, fileSystem.Root(), imported, error)) << error;
        EXPECT_EQ(imported.filename(), "Texture Copy.expa");

        std::filesystem::path destination;
        EXPECT_FALSE(fileSystem.Transfer(imported, externalDirectory, AssetFileTransferMode::move, destination, error));
        EXPECT_FALSE(error.empty());
        EXPECT_FALSE(fileSystem.Remove(fileSystem.Root(), error));
        EXPECT_TRUE(std::filesystem::exists(fileSystem.Root()));
    }

    TEST_F(AssetFileSystemTest, RejectsMovingFoldersIntoTheirDescendants)
    {
        AssetFileSystem fileSystem(assetDirectory);
        std::filesystem::path parent;
        std::filesystem::path child;
        std::string error;
        ASSERT_TRUE(fileSystem.CreateFolder(fileSystem.Root(), "Parent", parent, error)) << error;
        ASSERT_TRUE(fileSystem.CreateFolder(parent, "Child", child, error)) << error;

        std::filesystem::path destination;
        EXPECT_FALSE(fileSystem.Transfer(parent, child, AssetFileTransferMode::move, destination, error));
        EXPECT_TRUE(std::filesystem::exists(parent));
        EXPECT_FALSE(error.empty());
    }
}

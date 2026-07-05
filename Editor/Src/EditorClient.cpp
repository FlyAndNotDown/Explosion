//
// Created by johnk on 2026/7/4.
//

#include <array>

#include <Editor/EditorClient.h>
#include <Runtime/Asset/Asset.h>
#include <Runtime/Asset/Level.h>
#include <Runtime/Asset/Material.h>
#include <Runtime/Asset/Mesh.h>
#include <Runtime/Component/Player.h>
#include <Runtime/Component/Primitive.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/SystemGraphPresets.h>

namespace Editor::Internal {
    constexpr const char* mainLevelUri = "asset://Game/Maps/Main";
    constexpr const char* defaultUnlitMaterialUri = "asset://Game/Materials/DefaultUnlit";
    constexpr const char* defaultUnlitInstanceUri = "asset://Game/Materials/DefaultUnlitInstance";
    constexpr const char* cubeMeshUri = "asset://Game/Meshes/Cube";

    static bool AssetExists(const Core::Uri& inUri)
    {
        return Core::AssetUriParser(inUri).Parse().Exists();
    }

    template <typename A>
    static void SaveAsset(const Runtime::AssetPtr<A>& inAsset)
    {
        if (const Common::Path parentDir = Core::AssetUriParser(inAsset->Uri()).Parse().Parent();
            !parentDir.Exists()) {
            parentDir.MakeDir();
        }
        Runtime::AssetManager::Get().Save(inAsset);
    }

    static Runtime::StaticMeshVertices BuildCubeVertices()
    {
        // 24 vertices (4 per face) so each face gets its own uv space, z-up unit cube centered at origin
        static constexpr float h = 0.5f;
        static constexpr std::array<std::array<Common::FVec3, 4>, 6> facePositions = {{
            {{ { -h, -h, h }, { h, -h, h }, { h, h, h }, { -h, h, h } }},     // +z
            {{ { -h, h, -h }, { h, h, -h }, { h, -h, -h }, { -h, -h, -h } }}, // -z
            {{ { h, -h, -h }, { h, h, -h }, { h, h, h }, { h, -h, h } }},     // +x
            {{ { -h, h, -h }, { -h, -h, -h }, { -h, -h, h }, { -h, h, h } }}, // -x
            {{ { h, h, -h }, { -h, h, -h }, { -h, h, h }, { h, h, h } }},     // +y
            {{ { -h, -h, -h }, { h, -h, -h }, { h, -h, h }, { -h, -h, h } }} // -y
        }};

        Runtime::StaticMeshVertices result;
        for (const auto& face : facePositions) {
            const auto baseVertex = static_cast<uint32_t>(result.positions.size());
            for (size_t corner = 0; corner < 4; corner++) {
                result.positions.emplace_back(face[corner]);
                result.tangents.emplace_back(0.0f, 0.0f, 1.0f);
                result.uv0.emplace_back(corner == 1 || corner == 2 ? 1.0f : 0.0f, corner >= 2 ? 1.0f : 0.0f);
            }
            for (const uint32_t index : { 0u, 1u, 2u, 0u, 2u, 3u }) {
                result.indices.emplace_back(baseVertex + index);
            }
        }
        result.vertexCount = static_cast<uint32_t>(result.positions.size());
        result.indexCount = static_cast<uint32_t>(result.indices.size());
        return result;
    }

    static Runtime::AssetPtr<Runtime::StaticMesh> EnsureDefaultCubeMesh()
    {
        auto& assetManager = Runtime::AssetManager::Get();
        if (AssetExists(cubeMeshUri)) {
            return assetManager.SyncLoad<Runtime::StaticMesh>(cubeMeshUri, Mirror::Class::Get<Runtime::StaticMesh>());
        }

        Runtime::AssetPtr<Runtime::Material> material = new Runtime::Material(defaultUnlitMaterialUri);
        material->SetType(Runtime::MaterialType::surface);
        material->SetSource("float4 GetBaseColor()\n{\n    return baseColor;\n}\n");
        Runtime::MaterialFVec4ParameterField baseColorField;
        baseColorField.defaultValue = Common::FVec4(1.0f, 1.0f, 1.0f, 1.0f);
        material->EmplaceParameterField("baseColor") = baseColorField;
        material->Update();
        SaveAsset(material);

        Runtime::AssetPtr<Runtime::MaterialInstance> materialInstance = new Runtime::MaterialInstance(defaultUnlitInstanceUri);
        materialInstance->SetMaterial(material);
        materialInstance->SetParameter("baseColor", Common::FVec4(0.9f, 0.6f, 0.2f, 1.0f));
        SaveAsset(materialInstance);

        Runtime::AssetPtr<Runtime::StaticMesh> cubeMesh = new Runtime::StaticMesh(cubeMeshUri);
        cubeMesh->SetMaterial(materialInstance);
        cubeMesh->EmplaceLOD().vertices = BuildCubeVertices();
        SaveAsset(cubeMesh);
        return cubeMesh;
    }

    static void AuthorDefaultLevelContent(Runtime::ECRegistry& inRegistry)
    {
        const Runtime::AssetPtr<Runtime::StaticMesh> cubeMesh = EnsureDefaultCubeMesh();

        const auto ground = inRegistry.Create();
        Runtime::WorldTransform groundTransform;
        groundTransform.localToWorld.scale = Common::FVec3(10.0f, 10.0f, 0.2f);
        groundTransform.localToWorld.translation = Common::FVec3(0.0f, 0.0f, -0.1f);
        inRegistry.Emplace<Runtime::WorldTransform>(ground, groundTransform);
        auto& groundPrimitive = inRegistry.Emplace<Runtime::StaticPrimitive>(ground);
        groundPrimitive.mesh = cubeMesh;

        const auto cube = inRegistry.Create();
        Runtime::WorldTransform cubeTransform;
        cubeTransform.localToWorld.translation = Common::FVec3(0.0f, 0.0f, 0.5f);
        inRegistry.Emplace<Runtime::WorldTransform>(cube, cubeTransform);
        auto& cubePrimitive = inRegistry.Emplace<Runtime::StaticPrimitive>(cube);
        cubePrimitive.mesh = cubeMesh;

        const auto playerStart = inRegistry.Create();
        Runtime::WorldTransform playerStartTransform;
        playerStartTransform.localToWorld = Common::FTransform::LookAt(Common::FVec3(-5.0f, -6.0f, 4.0f), Common::FVec3(0.0f, 0.0f, 0.5f));
        inRegistry.Emplace<Runtime::WorldTransform>(playerStart, playerStartTransform);
        inRegistry.Emplace<Runtime::PlayerStart>(playerStart);
    }
}

namespace Editor {
    EditorClient::EditorClient()
        : levelUri(Internal::mainLevelUri)
        , world("EditorWorld", this, Runtime::PlayType::editor)
        , viewport(nullptr)
    {
        world.SetSystemGraph(Runtime::SystemGraphPresets::Default3DWorld());
    }

    EditorClient::~EditorClient()
    {
        if (world.Activated()) {
            world.Deactivate();
        }
    }

    Runtime::World& EditorClient::GetWorld()
    {
        return world;
    }

    Runtime::Viewport* EditorClient::GetViewport()
    {
        return viewport;
    }

    void EditorClient::SetViewport(Runtime::Viewport* inViewport)
    {
        viewport = inViewport;
    }

    void EditorClient::OpenProjectLevel()
    {
        Assert(!world.Activated());
        if (Internal::AssetExists(levelUri)) {
            const auto level = Runtime::AssetManager::Get().SyncLoad<Runtime::Level>(levelUri, Mirror::Class::Get<Runtime::Level>());
            world.LoadFrom(level);
            world.Activate();
        } else {
            Internal::AuthorDefaultLevelContent(world.GetRegistry());
            world.Activate();
            SaveLevel();
        }
    }

    void EditorClient::SaveLevel()
    {
        Runtime::AssetPtr<Runtime::Level> level = new Runtime::Level(levelUri);
        world.SaveTo(level);
        Internal::SaveAsset(level);
    }
}

//
// Created by johnk on 2026/7/4.
//

#include <array>
#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <Editor/EditorWindow.h>
#include <Editor/SceneClient.h>
#include <Runtime/Asset/Asset.h>
#include <Runtime/Asset/Level.h>
#include <Runtime/Asset/Material.h>
#include <Runtime/Asset/Mesh.h>
#include <Runtime/Component/Camera.h>
#include <Runtime/Component/Player.h>
#include <Runtime/Component/Primitive.h>
#include <Runtime/Component/Transform.h>
#include <Runtime/SystemGraphPresets.h>

namespace Editor::Internal {
    const Core::Uri mainLevelUri("asset://Game/Maps/Main");
    const Core::Uri defaultUnlitMaterialUri("asset://Game/Materials/DefaultUnlit");
    const Core::Uri defaultUnlitInstanceUri("asset://Game/Materials/DefaultUnlitInstance");
    const Core::Uri cubeMeshUri("asset://Game/Meshes/Cube");
    constexpr float cameraMoveSpeed = 5.0f;
    constexpr float cameraLookSpeedDegrees = 0.2f;
    constexpr float maxCameraPitchDegrees = 89.0f;
    constexpr float degToRad = 3.14159265358979323846f / 180.0f;

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
        static const std::array<std::array<Common::FVec3, 4>, 6> facePositions = {{
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

        // WorldTransform's reflected constructor takes an FTransform, other argument shapes would not match it
        const auto ground = inRegistry.Create();
        Common::FTransform groundTransform;
        groundTransform.scale = Common::FVec3(10.0f, 10.0f, 0.2f);
        groundTransform.translation = Common::FVec3(0.0f, 0.0f, -0.1f);
        inRegistry.Emplace<Runtime::WorldTransform>(ground, groundTransform);
        auto& groundPrimitive = inRegistry.Emplace<Runtime::StaticPrimitive>(ground);
        groundPrimitive.mesh = cubeMesh;

        const auto cube = inRegistry.Create();
        Common::FTransform cubeTransform;
        cubeTransform.translation = Common::FVec3(0.0f, 0.0f, 0.5f);
        inRegistry.Emplace<Runtime::WorldTransform>(cube, cubeTransform);
        auto& cubePrimitive = inRegistry.Emplace<Runtime::StaticPrimitive>(cube);
        cubePrimitive.mesh = cubeMesh;

        const auto playerStart = inRegistry.Create();
        const Common::FTransform playerStartTransform = Common::FTransform::LookAt(Common::FVec3(-5.0f, -6.0f, 4.0f), Common::FVec3(0.0f, 0.0f, 0.5f));
        inRegistry.Emplace<Runtime::WorldTransform>(playerStart, playerStartTransform);
        inRegistry.Emplace<Runtime::PlayerStart>(playerStart);
    }
}

namespace Editor {
    SceneClient::SceneClient()
        : levelUri(Internal::mainLevelUri)
        , world("EditorWorld", this, Runtime::PlayType::editor)
        , window(nullptr)
        , renderSurface(nullptr)
        , editorCamera(Runtime::entityNull)
        , sceneHovered(false)
        , cameraLooking(false)
        , cameraAnglesInitialized(false)
        , cameraYaw(0.0f)
        , cameraPitch(0.0f)
        , pendingLookDeltaX(0.0f)
        , pendingLookDeltaY(0.0f)
    {
        world.SetSystemGraph(Runtime::SystemGraphPresets::Default3DWorld());
    }

    SceneClient::~SceneClient()
    {
        EndCameraLook();
        if (!world.Stopped()) {
            world.Stop();
        }
    }

    Runtime::World& SceneClient::GetWorld()
    {
        return world;
    }

    Runtime::RenderSurface* SceneClient::GetRenderSurface()
    {
        return renderSurface;
    }

    void SceneClient::SetEditorWindow(EditorWindow& inWindow)
    {
        window = &inWindow;
    }

    void SceneClient::SetRenderSurface(Runtime::RenderSurface* inRenderSurface)
    {
        renderSurface = inRenderSurface;
    }

    void SceneClient::ResizeRenderSurface(uint32_t inWidth, uint32_t inHeight)
    {
        if (renderSurface == nullptr) {
            return;
        }
        if (window != nullptr) {
            window->WaitRenderingIdle();
        }
        renderSurface->Resize(inWidth, inHeight);
    }

    void SceneClient::OpenProjectLevel()
    {
        Assert(world.Stopped());
        if (Internal::AssetExists(levelUri)) {
            const auto level = Runtime::AssetManager::Get().SyncLoad<Runtime::Level>(levelUri, Mirror::Class::Get<Runtime::Level>());
            world.LoadFrom(level);
        } else {
            Internal::AuthorDefaultLevelContent(world.GetRegistry());
            SaveLevel();
        }
        CreateEditorCamera();
        world.Play();
    }

    void SceneClient::SaveLevel()
    {
        Runtime::AssetPtr<Runtime::Level> level = new Runtime::Level(levelUri);
        world.SaveTo(level);
        Internal::SaveAsset(level);
    }

    Runtime::Entity SceneClient::GetEditorCamera() const
    {
        return editorCamera;
    }

    void SceneClient::TickEditorCamera(float inDeltaSeconds)
    {
        if (!world.Playing() || editorCamera == Runtime::entityNull) {
            return;
        }
        auto& registry = world.GetRegistry();

        if (!cameraAnglesInitialized) {
            const auto forward = registry.Get<Runtime::WorldTransform>(editorCamera).localToWorld.GetRotationMatrix().Col(0);
            cameraYaw = std::atan2(-forward.y, forward.x);
            const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
            cameraPitch = std::clamp(std::asin(std::clamp(forward.z, -1.0f, 1.0f)), -maxPitch, maxPitch);
            cameraAnglesInitialized = true;
        }

        if (cameraLooking) {
            cameraYaw -= pendingLookDeltaX * Internal::cameraLookSpeedDegrees * Internal::degToRad;
            cameraPitch -= pendingLookDeltaY * Internal::cameraLookSpeedDegrees * Internal::degToRad;
            const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
            cameraPitch = std::clamp(cameraPitch, -maxPitch, maxPitch);
            pendingLookDeltaX = 0.0f;
            pendingLookDeltaY = 0.0f;
        }

        const Common::FVec3 moveInput = cameraLooking ? CameraMoveInput() : Common::FVec3Consts::zero;
        const bool moving = moveInput.x != 0.0f || moveInput.y != 0.0f || moveInput.z != 0.0f;
        if (!moving && !cameraLooking) {
            return;
        }

        const Common::FQuat orientation =
            Common::FQuat(Common::FVec3Consts::unitY, Common::FRadian(cameraPitch))
            * Common::FQuat(Common::FVec3Consts::unitZ, Common::FRadian(cameraYaw));
        const Common::FVec3 lookForward = orientation.RotateVector(Common::FVec3Consts::unitX);
        const Common::FVec3 lookRight = orientation.RotateVector(Common::FVec3Consts::unitY);
        Common::FVec3 moveForward(lookForward.x, lookForward.y, 0.0f);
        Common::FVec3 moveRight(lookRight.x, lookRight.y, 0.0f);
        moveForward.Normalize();
        moveRight.Normalize();

        registry.Update<Runtime::WorldTransform>(editorCamera, [&](Runtime::WorldTransform& transform) -> void {
            const float moveDelta = Internal::cameraMoveSpeed * inDeltaSeconds;
            transform.localToWorld.translation += moveForward * (moveInput.x * moveDelta);
            transform.localToWorld.translation += moveRight * (moveInput.y * moveDelta);
            transform.localToWorld.translation += Common::FVec3(0.0f, 0.0f, 1.0f) * (moveInput.z * moveDelta);
            transform.localToWorld.rotation = orientation;
        });
    }

    void SceneClient::SetSceneHovered(bool inHovered)
    {
        sceneHovered = inHovered;
    }

    void SceneClient::SetSceneFocused(bool inFocused)
    {
        if (!inFocused && !cameraLooking) {
            pressedKeys.clear();
        }
    }

    bool SceneClient::IsSceneHovered() const
    {
        return sceneHovered;
    }

    bool SceneClient::IsCameraLooking() const
    {
        return cameraLooking;
    }

    void SceneClient::OnKey(int inKey, bool inPressed)
    {
        if (inPressed) {
            pressedKeys.insert(inKey);
        } else {
            pressedKeys.erase(inKey);
        }
    }

    void SceneClient::BeginCameraLook()
    {
        if (cameraLooking || !sceneHovered) {
            return;
        }
        cameraLooking = true;
        if (window != nullptr) {
            glfwSetInputMode(window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    void SceneClient::EndCameraLook()
    {
        if (!cameraLooking) {
            return;
        }
        cameraLooking = false;
        pendingLookDeltaX = 0.0f;
        pendingLookDeltaY = 0.0f;
        pressedKeys.clear();
        if (window != nullptr) {
            glfwSetInputMode(window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void SceneClient::AddCameraLookDelta(float inDeltaX, float inDeltaY)
    {
        if (!cameraLooking) {
            return;
        }
        pendingLookDeltaX += inDeltaX;
        pendingLookDeltaY += inDeltaY;
    }

    void SceneClient::CreateEditorCamera()
    {
        auto& registry = world.GetRegistry();
        editorCamera = registry.Create();
        registry.Emplace<Runtime::TransientTag>(editorCamera);
        registry.Emplace<Runtime::Camera>(editorCamera);
        // WorldTransform's reflected constructor takes an FTransform, other argument shapes would not match it
        const Common::FTransform cameraTransform = Common::FTransform::LookAt(Common::FVec3(-5.0f, -6.0f, 4.0f), Common::FVec3(0.0f, 0.0f, 0.5f));
        registry.Emplace<Runtime::WorldTransform>(editorCamera, cameraTransform);
    }

    Common::FVec3 SceneClient::CameraMoveInput() const
    {
        Common::FVec3 result(0.0f, 0.0f, 0.0f);
        if (pressedKeys.contains(GLFW_KEY_W)) { result.x += 1.0f; }
        if (pressedKeys.contains(GLFW_KEY_S)) { result.x -= 1.0f; }
        if (pressedKeys.contains(GLFW_KEY_D)) { result.y += 1.0f; }
        if (pressedKeys.contains(GLFW_KEY_A)) { result.y -= 1.0f; }
        if (pressedKeys.contains(GLFW_KEY_E)) { result.z += 1.0f; }
        if (pressedKeys.contains(GLFW_KEY_Q)) { result.z -= 1.0f; }
        if (result.Model() > 1.0f) {
            result.Normalize();
        }
        return result;
    }
}

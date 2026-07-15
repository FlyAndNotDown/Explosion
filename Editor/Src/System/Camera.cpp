#include <algorithm>
#include <cmath>

#include <Editor/System/Camera.h>
#include <Runtime/Component/Transform.h>

namespace Editor::Internal {
    constexpr float cameraMoveSpeed = 5.0f;
    constexpr float cameraLookSpeedDegrees = 0.2f;
    constexpr float maxCameraPitchDegrees = 89.0f;
    constexpr float degToRad = 3.14159265358979323846f / 180.0f;

    static Common::FVec2 GetMoveInput(const EditorCameraInput& inInput)
    {
        Common::FVec2 result(
            static_cast<float>(inInput.moveForward) - static_cast<float>(inInput.moveBackward),
            static_cast<float>(inInput.moveRight) - static_cast<float>(inInput.moveLeft));
        if (result.Model() > 1.0f) {
            result.Normalize();
        }
        return result;
    }
}

namespace Editor {
    EditorCameraInput::EditorCameraInput()
        : moveForward(false)
        , moveBackward(false)
        , moveLeft(false)
        , moveRight(false)
        , looking(false)
        , lookDelta(Common::FVec2Consts::zero)
    {
    }

    EditorCameraController::EditorCameraController()
        : anglesInitialized(false)
        , yaw(0.0f)
        , pitch(0.0f)
    {
    }

    EditorCameraSystem::EditorCameraSystem(Runtime::ECRegistry& inRegistry, const Runtime::SystemSetupContext& inContext)
        : System(inRegistry, inContext)
    {
        registry.GEmplace<EditorCameraInput>();
    }

    EditorCameraSystem::~EditorCameraSystem()
    {
        registry.GRemove<EditorCameraInput>();
    }

    void EditorCameraSystem::Tick(float inDeltaTimeSeconds)
    {
        auto& input = registry.GGet<EditorCameraInput>();
        const Common::FVec2 moveInput = input.looking ? Internal::GetMoveInput(input) : Common::FVec2Consts::zero;
        const bool moving = moveInput.x != 0.0f || moveInput.y != 0.0f;
        const bool looking = input.looking && (input.lookDelta.x != 0.0f || input.lookDelta.y != 0.0f);

        registry.View<EditorCameraController, Runtime::WorldTransform>().Each(
            [&](Runtime::Entity entity, EditorCameraController& controller, Runtime::WorldTransform& worldTransform) -> void {
                if (!controller.anglesInitialized) {
                    const auto forward = worldTransform.localToWorld.GetRotationMatrix().Col(0);
                    controller.yaw = std::atan2(-forward.y, forward.x);
                    const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
                    controller.pitch = std::clamp(std::asin(std::clamp(forward.z, -1.0f, 1.0f)), -maxPitch, maxPitch);
                    controller.anglesInitialized = true;
                }

                if (looking) {
                    controller.yaw -= input.lookDelta.x * Internal::cameraLookSpeedDegrees * Internal::degToRad;
                    controller.pitch -= input.lookDelta.y * Internal::cameraLookSpeedDegrees * Internal::degToRad;
                    const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
                    controller.pitch = std::clamp(controller.pitch, -maxPitch, maxPitch);
                }
                if (!moving && !looking) {
                    return;
                }

                const Common::FQuat orientation =
                    Common::FQuat(Common::FVec3Consts::unitY, Common::FRadian(controller.pitch))
                    * Common::FQuat(Common::FVec3Consts::unitZ, Common::FRadian(controller.yaw));
                const Common::FVec3 moveForward = orientation.RotateVector(Common::FVec3Consts::unitX);
                const Common::FVec3 moveRight = orientation.RotateVector(Common::FVec3Consts::unitY);
                registry.Update<Runtime::WorldTransform>(entity, [&](Runtime::WorldTransform& transform) -> void {
                    const float moveDelta = Internal::cameraMoveSpeed * inDeltaTimeSeconds;
                    transform.localToWorld.translation += moveForward * (moveInput.x * moveDelta);
                    transform.localToWorld.translation += moveRight * (moveInput.y * moveDelta);
                    transform.localToWorld.rotation = orientation;
                });
            });

        input.lookDelta = Common::FVec2Consts::zero;
    }
}

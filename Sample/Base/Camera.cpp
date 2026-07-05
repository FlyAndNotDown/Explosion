//
// Created by johnk on 2024/5/20.
//

#include <algorithm>

#include <Camera.h>

static constexpr float maxPitchDeg = 89.0f;

Camera::Camera(const FVec3& inPos, float inPitchDeg, float inYawDeg, const ProjectionParams& inProjParams)
    : viewTransform(FQuatConsts::identity, inPos)
    , projection(inProjParams.fov, inProjParams.width, inProjParams.height, inProjParams.nearPlane, inProjParams.farPlane)
    , pitchDeg(inPitchDeg)
    , yawDeg(inYawDeg)
    , movingStatus()
    , moveSpeed(1.0f)
    , rotateSpeed(1.0f)
{
    RebuildRotation();

    moveVectorMap = {
        { MoveDirection::front, FVec3Consts::unitX },
        { MoveDirection::back, FVec3Consts::negaUnitX },
        { MoveDirection::left, FVec3Consts::negaUnitY },
        { MoveDirection::right, FVec3Consts::unitY },
        { MoveDirection::up, FVec3Consts::unitZ },
        { MoveDirection::down, FVec3Consts::negaUnitZ }
    };
}

void Camera::SetPosition(const FVec3& inPosition)
{
    viewTransform.translation = inPosition;
}

void Camera::SetRotation(float inPitchDeg, float inYawDeg)
{
    pitchDeg = inPitchDeg;
    yawDeg = inYawDeg;
    RebuildRotation();
}

void Camera::SetTranslation(const FVec3& inTranslation)
{
    viewTransform.translation = inTranslation;
}

void Camera::Translate(const FVec3& inTransDelta)
{
    // inTransDelta is in camera's local, needs to be changed to world
    viewTransform.translation += viewTransform.rotation.RotateVector(inTransDelta);
}

void Camera::Rotate(float inPitchDeltaDeg, float inYawDeltaDeg)
{
    pitchDeg += inPitchDeltaDeg;
    yawDeg += inYawDeltaDeg;
    RebuildRotation();
}

void Camera::PerformMove(MoveDirection direction)
{
    movingStatus[static_cast<size_t>(direction)] = true;
}

void Camera::PerformStop(MoveDirection direction)
{
    movingStatus[static_cast<size_t>(direction)] = false;
}

void Camera::SetMoveSpeed(float inSpeed)
{
    moveSpeed = inSpeed;
}

void Camera::SetRotateSpeed(float inSpeed)
{
    rotateSpeed = inSpeed;
}

void Camera::Update(float frameTime)
{
    if (!Moving()) {
        return;
    }

    const float frameMovement = moveSpeed * frameTime;
    FVec3 posDelta = FVec3Consts::zero;
    for (auto i = 0; i < movingStatus.size(); i++) {
        if (movingStatus[i]) {
            posDelta += moveVectorMap.at(static_cast<MoveDirection>(i)) * frameMovement;
        }
    }
    Translate(posDelta);
}

FMat4x4 Camera::GetProjectionMatrix() const
{
    return projection.GetProjectionMatrix();
}

FMat4x4 Camera::GetViewMatrix() const
{
    return viewTransform.GetViewMatrix();
}

float Camera::GetMoveSpeed() const
{
    return moveSpeed;
}

float Camera::GetRotateSpeed() const
{
    return rotateSpeed;
}

bool Camera::Moving() const
{
    bool result = false;
    for (auto i = 0; i < movingStatus.size(); i++) {
        result = result || movingStatus[i];
    }
    return result;
}

void Camera::RebuildRotation()
{
    pitchDeg = std::clamp(pitchDeg, -maxPitchDeg, maxPitchDeg);
    viewTransform.rotation = FQuat(FVec3Consts::unitY, pitchDeg) * FQuat(FVec3Consts::unitZ, yawDeg);
}

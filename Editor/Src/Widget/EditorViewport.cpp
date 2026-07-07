//
// Created by johnk on 2026/7/4.
//

#include <algorithm>
#include <cmath>

#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>

#include <Editor/Widget/EditorViewport.h>
#include <Editor/Widget/moc_EditorViewport.cpp> // NOLINT
#include <Runtime/Component/Transform.h>
#include <Runtime/Engine.h>

// unity builds can pull windows.h (via httplib/qt) into this translation unit after the RHI headers already ran
// their cleanup, so the win32 semaphore macro would mangle RHI::Device::CreateSemaphore below
#undef CreateSemaphore

namespace Editor::Internal {
    constexpr float cameraMoveSpeed = 5.0f;
    constexpr float cameraLookSpeedDegrees = 0.2f;
    constexpr float maxCameraPitchDegrees = 89.0f;
    constexpr float degToRad = 3.14159265358979323846f / 180.0f;
}

namespace Editor {
    EditorViewport::EditorViewport(EditorClient& inClient, QWidget* inParent)
        : GraphicsWidget(inParent)
        , client(inClient)
        , imageReadySemaphore(GetDevice().CreateSemaphore())
        , renderFinishedSemaphore(GetDevice().CreateSemaphore())
        , cameraLooking(false)
        , cameraAnglesInitialized(false)
        , cameraYaw(0.0f)
        , cameraPitch(0.0f)
    {
        setMinimumSize(320, 240);
        setFocusPolicy(Qt::StrongFocus);
        RecreateSwapChain(GetWidth(), GetHeight());
        client.SetViewport(this);
    }

    EditorViewport::~EditorViewport()
    {
        client.SetViewport(nullptr);
        WaitRenderingIdle();
        swapChain.Reset();
    }

    Runtime::Client& EditorViewport::GetClient()
    {
        return client;
    }

    Runtime::PresentInfo EditorViewport::GetNextPresentInfo()
    {
        const auto backTextureIndex = swapChain->AcquireBackTexture(imageReadySemaphore.Get());

        Runtime::PresentInfo result;
        result.backTexture = swapChain->GetTexture(backTextureIndex);
        result.imageReadySemaphore = imageReadySemaphore.Get();
        result.renderFinishedSemaphore = renderFinishedSemaphore.Get();
        return result;
    }

    uint32_t EditorViewport::GetWidth() const
    {
        // physical pixels: the swapchain (and thus the render targets) live in device pixel space
        return static_cast<uint32_t>(std::max(1.0, width() * devicePixelRatioF()));
    }

    uint32_t EditorViewport::GetHeight() const
    {
        return static_cast<uint32_t>(std::max(1.0, height() * devicePixelRatioF()));
    }

    void EditorViewport::Resize(uint32_t inWidth, uint32_t inHeight)
    {
        RecreateSwapChain(inWidth, inHeight);
    }

    void EditorViewport::Present(const Runtime::PresentInfo& inPresentInfo)
    {
        swapChain->Present(inPresentInfo.renderFinishedSemaphore);
    }

    void EditorViewport::TickEditorCamera(float inDeltaSeconds)
    {
        auto& world = client.GetWorld();
        const auto cameraEntity = client.GetEditorCamera();
        if (!world.Playing() || cameraEntity == Runtime::entityNull) {
            return;
        }
        auto& registry = world.GetRegistry();

        if (!cameraAnglesInitialized) {
            // entity forward is local +x; the no-roll rig is Qy(pitch)*Qz(yaw) (yaw about world +z outermost) whose
            // forward column is (cosP cosY, -cosP sinY, sinP), so recover the angles matching that sign convention
            const auto forward = registry.Get<Runtime::WorldTransform>(cameraEntity).localToWorld.GetRotationMatrix().Col(0);
            cameraYaw = std::atan2(-forward.y, forward.x);
            const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
            cameraPitch = std::clamp(std::asin(std::clamp(forward.z, -1.0f, 1.0f)), -maxPitch, maxPitch);
            cameraAnglesInitialized = true;
        }

        const Common::FVec3 moveInput = cameraLooking ? CameraMoveInput() : Common::FVec3Consts::zero;
        const bool moving = moveInput.x != 0.0f || moveInput.y != 0.0f || moveInput.z != 0.0f;
        if (!moving && !cameraLooking) {
            return;
        }

        // rebuild the orientation from the absolute pitch/yaw pair exactly like Sample/Base/Camera: composing yaw about
        // the world up axis outermost keeps the right vector horizontal, so the view can never accumulate roll
        const Common::FQuat orientation =
            Common::FQuat(Common::FVec3Consts::unitY, Common::FRadian(cameraPitch))
            * Common::FQuat(Common::FVec3Consts::unitZ, Common::FRadian(cameraYaw));
        const Common::FVec3 lookForward = orientation.RotateVector(Common::FVec3Consts::unitX);
        const Common::FVec3 lookRight = orientation.RotateVector(Common::FVec3Consts::unitY);
        Common::FVec3 moveForward(lookForward.x, lookForward.y, 0.0f);
        Common::FVec3 moveRight(lookRight.x, lookRight.y, 0.0f);
        moveForward.Normalize();
        moveRight.Normalize();

        registry.Update<Runtime::WorldTransform>(cameraEntity, [&](Runtime::WorldTransform& transform) -> void {
            const float moveDelta = Internal::cameraMoveSpeed * inDeltaSeconds;
            transform.localToWorld.translation += moveForward * (moveInput.x * moveDelta);
            transform.localToWorld.translation += moveRight * (moveInput.y * moveDelta);
            transform.localToWorld.translation += Common::FVec3(0.0f, 0.0f, 1.0f) * (moveInput.z * moveDelta);
            transform.localToWorld.rotation = orientation;
        });
    }

    void EditorViewport::resizeEvent(QResizeEvent* inEvent)
    {
        GraphicsWidget::resizeEvent(inEvent);
        if (const QSize size = inEvent->size();
            swapChain.Valid() && size.width() > 0 && size.height() > 0) {
            RecreateSwapChain(GetWidth(), GetHeight());
        }
    }

    bool EditorViewport::event(QEvent* inEvent)
    {
        // docking/floating the widget can recreate its native window, the surface and swapchain bound to the old
        // window handle must be rebuilt on the new one
        if (inEvent->type() == QEvent::WinIdChange && swapChain.Valid()) {
            RecreateSurfaceAndSwapChain();
        }
        return GraphicsWidget::event(inEvent);
    }

    void EditorViewport::keyPressEvent(QKeyEvent* inEvent)
    {
        if (!inEvent->isAutoRepeat()) {
            pressedKeys.insert(inEvent->key());
        }
        GraphicsWidget::keyPressEvent(inEvent);
    }

    void EditorViewport::keyReleaseEvent(QKeyEvent* inEvent)
    {
        if (!inEvent->isAutoRepeat()) {
            pressedKeys.erase(inEvent->key());
        }
        GraphicsWidget::keyReleaseEvent(inEvent);
    }

    void EditorViewport::mousePressEvent(QMouseEvent* inEvent)
    {
        if (inEvent->button() == Qt::RightButton) {
            BeginCameraLook();
            inEvent->accept();
            return;
        }
        GraphicsWidget::mousePressEvent(inEvent);
    }

    void EditorViewport::mouseMoveEvent(QMouseEvent* inEvent)
    {
        if (cameraLooking) {
            const QPoint center = rect().center();
            const QPoint delta = inEvent->pos() - center;
            if (!delta.isNull()) {
                cameraYaw -= static_cast<float>(delta.x()) * Internal::cameraLookSpeedDegrees * Internal::degToRad;
                cameraPitch -= static_cast<float>(delta.y()) * Internal::cameraLookSpeedDegrees * Internal::degToRad;
                const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
                cameraPitch = std::clamp(cameraPitch, -maxPitch, maxPitch);
                QCursor::setPos(mapToGlobal(center));
            }
            inEvent->accept();
            return;
        }
        GraphicsWidget::mouseMoveEvent(inEvent);
    }

    void EditorViewport::mouseReleaseEvent(QMouseEvent* inEvent)
    {
        if (inEvent->button() == Qt::RightButton) {
            EndCameraLook();
            inEvent->accept();
            return;
        }
        GraphicsWidget::mouseReleaseEvent(inEvent);
    }

    void EditorViewport::focusOutEvent(QFocusEvent* inEvent)
    {
        pressedKeys.clear();
        EndCameraLook();
        GraphicsWidget::focusOutEvent(inEvent);
    }

    void EditorViewport::RecreateSwapChain(uint32_t inWidth, uint32_t inHeight)
    {
        static std::vector<RHI::PixelFormat> formatQualifiers = {
            RHI::PixelFormat::rgba8Unorm,
            RHI::PixelFormat::bgra8Unorm
        };

        WaitRenderingIdle();
        if (swapChain.Valid()) {
            swapChain.Reset();
        }

        std::optional<RHI::PixelFormat> pixelFormat = {};
        for (const auto format : formatQualifiers) {
            if (GetDevice().CheckSwapChainFormatSupport(&GetSurface(), format, RHI::ColorSpace::srgbNonLinear)) {
                pixelFormat = format;
                break;
            }
        }
        Assert(pixelFormat.has_value());

        swapChain = GetDevice().CreateSwapChain(
            RHI::SwapChainCreateInfo()
                .SetPresentQueue(GetDevice().GetQueue(RHI::QueueType::graphics, 0))
                .SetSurface(&GetSurface())
                .SetTextureNum(2)
                .SetFormat(pixelFormat.value())
                .SetWidth(inWidth)
                .SetHeight(inHeight)
                .SetPresentMode(RHI::PresentMode::immediately));
    }

    void EditorViewport::RecreateSurfaceAndSwapChain()
    {
        WaitRenderingIdle();
        swapChain.Reset();
        surface = device->CreateSurface(
            RHI::SurfaceCreateInfo()
                .SetWindow(reinterpret_cast<void*>(winId()))); // NOLINT
        RecreateSwapChain(GetWidth(), GetHeight());
    }

    void EditorViewport::WaitRenderingIdle() const
    {
        Runtime::EngineHolder::Get().GetRenderModule().GetRenderThread().Flush();
        WaitDeviceIdle();
    }

    void EditorViewport::BeginCameraLook()
    {
        if (cameraLooking) {
            return;
        }
        cameraLooking = true;
        cameraCursorRestorePos = QCursor::pos();
        setFocus(Qt::MouseFocusReason);
        grabMouse(QCursor(Qt::BlankCursor));
        QCursor::setPos(mapToGlobal(rect().center()));
    }

    void EditorViewport::EndCameraLook()
    {
        if (!cameraLooking) {
            return;
        }
        cameraLooking = false;
        releaseMouse();
        unsetCursor();
        QCursor::setPos(cameraCursorRestorePos);
    }

    Common::FVec3 EditorViewport::CameraMoveInput() const
    {
        Common::FVec3 result(0.0f, 0.0f, 0.0f);
        if (pressedKeys.contains(Qt::Key_W)) { result.x += 1.0f; }
        if (pressedKeys.contains(Qt::Key_S)) { result.x -= 1.0f; }
        if (pressedKeys.contains(Qt::Key_D)) { result.y += 1.0f; }
        if (pressedKeys.contains(Qt::Key_A)) { result.y -= 1.0f; }
        if (pressedKeys.contains(Qt::Key_E)) { result.z += 1.0f; }
        if (pressedKeys.contains(Qt::Key_Q)) { result.z -= 1.0f; }
        if (result.Model() > 1.0f) {
            result.Normalize();
        }
        return result;
    }
}

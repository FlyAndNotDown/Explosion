//
// Created by johnk on 2026/7/4.
//

#include <algorithm>
#include <cmath>

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

    static Common::FVec3 DirectionFromYawPitch(float inYawRadians, float inPitchRadians)
    {
        return {
            std::cos(inPitchRadians) * std::cos(inYawRadians),
            std::cos(inPitchRadians) * std::sin(inYawRadians),
            std::sin(inPitchRadians)
        };
    }
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
            // the entity forward axis is local +x, see Transform::UpdateRotation
            const auto forward = registry.Get<Runtime::WorldTransform>(cameraEntity).localToWorld.GetRotationMatrix().Col(0);
            cameraYaw = std::atan2(forward.y, forward.x);
            cameraPitch = std::asin(std::clamp(forward.z, -1.0f, 1.0f));
            cameraAnglesInitialized = true;
        }

        const Common::FVec3 moveInput = CameraMoveInput();
        const bool moving = moveInput.x != 0.0f || moveInput.y != 0.0f || moveInput.z != 0.0f;
        if (!moving && !cameraLooking) {
            return;
        }

        const Common::FVec3 forward = Internal::DirectionFromYawPitch(cameraYaw, cameraPitch);
        Common::FVec3 right = Common::FVec3(0.0f, 0.0f, 1.0f).Cross(forward);
        right.Normalize();

        registry.Update<Runtime::WorldTransform>(cameraEntity, [&](Runtime::WorldTransform& transform) -> void {
            const float moveDelta = Internal::cameraMoveSpeed * inDeltaSeconds;
            transform.localToWorld.translation += forward * (moveInput.x * moveDelta);
            transform.localToWorld.translation += right * (moveInput.y * moveDelta);
            transform.localToWorld.translation += Common::FVec3(0.0f, 0.0f, 1.0f) * (moveInput.z * moveDelta);
            transform.localToWorld.LookTo(transform.localToWorld.translation + forward);
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
            cameraLooking = true;
            lastMousePos = inEvent->pos();
            setCursor(Qt::BlankCursor);
        }
        GraphicsWidget::mousePressEvent(inEvent);
    }

    void EditorViewport::mouseMoveEvent(QMouseEvent* inEvent)
    {
        if (cameraLooking) {
            const QPoint delta = inEvent->pos() - lastMousePos;
            lastMousePos = inEvent->pos();

            cameraYaw -= static_cast<float>(delta.x()) * Internal::cameraLookSpeedDegrees * Internal::degToRad;
            cameraPitch -= static_cast<float>(delta.y()) * Internal::cameraLookSpeedDegrees * Internal::degToRad;
            const float maxPitch = Internal::maxCameraPitchDegrees * Internal::degToRad;
            cameraPitch = std::clamp(cameraPitch, -maxPitch, maxPitch);
        }
        GraphicsWidget::mouseMoveEvent(inEvent);
    }

    void EditorViewport::mouseReleaseEvent(QMouseEvent* inEvent)
    {
        if (inEvent->button() == Qt::RightButton) {
            cameraLooking = false;
            unsetCursor();
        }
        GraphicsWidget::mouseReleaseEvent(inEvent);
    }

    void EditorViewport::focusOutEvent(QFocusEvent* inEvent)
    {
        pressedKeys.clear();
        cameraLooking = false;
        unsetCursor();
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

    Common::FVec3 EditorViewport::CameraMoveInput() const
    {
        Common::FVec3 result(0.0f, 0.0f, 0.0f);
        if (pressedKeys.contains(Qt::Key_W)) { result.x += 1.0f; }
        if (pressedKeys.contains(Qt::Key_S)) { result.x -= 1.0f; }
        if (pressedKeys.contains(Qt::Key_D)) { result.y += 1.0f; }
        if (pressedKeys.contains(Qt::Key_A)) { result.y -= 1.0f; }
        if (pressedKeys.contains(Qt::Key_E)) { result.z += 1.0f; }
        if (pressedKeys.contains(Qt::Key_Q)) { result.z -= 1.0f; }
        return result;
    }
}

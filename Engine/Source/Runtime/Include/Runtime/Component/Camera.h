//
// Created by johnk on 2024/10/14.
//

#pragma once

#include <optional>

#include <Render/View.h>
#include <Runtime/Meta.h>
#include <Runtime/Api.h>
#include <Runtime/RenderThreadPtr.h>

namespace Runtime {
    struct RUNTIME_API EClass(comp, transient) Camera final {
        EClassBody(Camera)

        Camera();

        EProperty() bool perspective;
        EProperty() float nearPlane;
        EProperty() std::optional<float> farPlane;
        // only need when perspective
        EProperty() std::optional<float> fov;
        // cross-frame rendering history of the view rendered through this camera, allocated by RenderSystem on first
        // use and released on the render thread with the component
        RenderThreadPtr<Render::ViewState> viewState;
    };

    // TODO scene capture
}

//
// Created by johnk on 2024/10/14.
//

#pragma once

#include <optional>
#include <cstdint>
#include <limits>

#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    using ViewIndex = uint8_t;
    constexpr ViewIndex viewIndexNull = std::numeric_limits<uint8_t>::max();

    struct RUNTIME_API EClass(transient) Camera final {
        EClassBody(Camera)

        Camera();

        EProperty() ViewIndex viewIndex;
        EProperty() bool perspective;
        EProperty() float nearPlane;
        EProperty() std::optional<float> farPlane;
        // only need when perspective
        EProperty() std::optional<float> fov;
    };

    // TODO scene capture
}

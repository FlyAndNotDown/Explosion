//
// Created by johnk on 2023/8/14.
//

#pragma once

#include <Common/Math/Transform.h>
#include <Common/Math/Color.h>

namespace Render {
    struct LightSceneProxy {
        LightSceneProxy();

        Common::FMat4x4 localToWorld;
        Common::Color color;
        float intensity;
    };

    struct DirectionalLightSceneProxy final : LightSceneProxy {
        DirectionalLightSceneProxy();
    };

    struct PointLightSceneProxy final : LightSceneProxy {
        PointLightSceneProxy();

        float radius;
    };

    struct SpotLightSceneProxy final : LightSceneProxy {
        SpotLightSceneProxy();
    };
}

namespace Render {
    inline LightSceneProxy::LightSceneProxy()
        : localToWorld(Common::FMat4x4Consts::identity)
        , color(Common::ColorConsts::white)
        , intensity(0.0f)
    {
    }

    inline DirectionalLightSceneProxy::DirectionalLightSceneProxy() = default;

    inline PointLightSceneProxy::PointLightSceneProxy()
        : radius(0.0f)
    {
    }

    inline SpotLightSceneProxy::SpotLightSceneProxy() = default;
}

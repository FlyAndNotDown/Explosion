//
// Created by johnk on 2024/10/14.
//

#pragma once

#include <Common/Math/Adapters.h>
#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    struct RUNTIME_API EClass(comp) DirectionalLight final {
        EClassBody(DirectionalLight)

        DirectionalLight();

        EProperty() Common::Color color;
        EProperty() float intensity;
        EProperty() bool castShadows;
    };

    struct RUNTIME_API EClass(comp) PointLight final {
        EClassBody(PointLight)

        PointLight();

        EProperty() Common::Color color;
        EProperty() float intensity;
        EProperty() bool castShadows;
        EProperty() float radius;
    };

    struct RUNTIME_API EClass(comp) SpotLight final {
        EClassBody(SpotLight)

        SpotLight();

        EProperty() Common::Color color;
        EProperty() float intensity;
        EProperty() bool castShadows;
    };
}

//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <string>

#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    struct RUNTIME_API EClass(comp) Name final {
        EClassBody(Name)

        Name();
        explicit Name(std::string inValue);

        EProperty() std::string value;
    };
}

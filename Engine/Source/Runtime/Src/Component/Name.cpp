//
// Created by johnk on 2026/7/5.
//

#include <Runtime/Component/Name.h>

namespace Runtime {
    Name::Name() = default;

    Name::Name(std::string inValue)
        : value(std::move(inValue))
    {
    }
}

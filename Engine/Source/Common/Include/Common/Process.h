#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Common {
    class Process {
    public:
        static std::optional<int32_t> Run(const std::string& inExecutablePath, const std::vector<std::string>& inArguments);
    };
}

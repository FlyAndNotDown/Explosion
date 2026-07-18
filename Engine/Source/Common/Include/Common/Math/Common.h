//
// Created by johnk on 2023/5/10.
//

#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include <Common/Concepts.h>

namespace Common {
    static constexpr float epsilon = 0.000001f;
    static constexpr float halfEpsilon = 0.001f;
    static constexpr float pi = std::numbers::pi_v<float>;
}

namespace Common {
    template <typename T> T DefaultTolerance();
    template <typename T> T Pi();
    template <typename T> requires std::is_floating_point_v<T> bool AlmostEqual(T lhs, T rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());
    template <typename T> bool CompareNumber(T lhs, T rhs);
    template <CppIntegral T> T DivideAndRoundUp(T lhs, T rhs);
}

namespace Common {
    template <typename T>
    T DefaultTolerance()
    {
        if constexpr (std::is_same_v<T, float>) {
            return static_cast<T>(epsilon);
        } else if constexpr (std::is_same_v<T, double>) {
            return static_cast<T>(1e-12);
        } else if constexpr (std::is_same_v<T, long double>) {
            return static_cast<T>(1e-15L);
        } else {
            return static_cast<T>(halfEpsilon);
        }
    }

    template <typename T>
    T Pi()
    {
        if constexpr (std::is_floating_point_v<T>) {
            return std::numbers::pi_v<T>;
        } else {
            return static_cast<T>(std::numbers::pi_v<float>);
        }
    }

    template <typename T>
    requires std::is_floating_point_v<T>
    bool AlmostEqual(T lhs, T rhs, T absoluteTolerance, T relativeTolerance)
    {
        if (lhs == rhs) {
            return true;
        }
        if (!std::isfinite(lhs) || !std::isfinite(rhs)) {
            return false;
        }

        const T difference = std::abs(lhs - rhs);
        const T scale = std::max(std::abs(lhs), std::abs(rhs));
        return difference <= std::max(absoluteTolerance, relativeTolerance * scale);
    }

    template <typename T>
    bool CompareNumber(T lhs, T rhs)
    {
        if constexpr (std::is_floating_point_v<T>) {
            const T tolerance = DefaultTolerance<T>();
            return AlmostEqual(lhs, rhs, tolerance, tolerance);
        } else {
            return lhs == rhs;
        }
    }

    template <CppIntegral T>
    T DivideAndRoundUp(T lhs, T rhs)
    {
        return (lhs + rhs - 1) / rhs;
    }
}

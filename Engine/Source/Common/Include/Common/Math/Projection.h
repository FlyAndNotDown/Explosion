//
// Created by johnk on 2023/6/26.
//

#pragma once

#include <optional>

#include <Common/Math/Matrix.h>
#include <Common/Math/Quaternion.h>

namespace Common {
    template <FloatingPoint T>
    struct ReversedZOrthogonalProjectionBase {
        T width;
        T height;
        T nearPlane;
        std::optional<T> farPlane;
    };

    template <FloatingPoint T>
    struct ReversedZPerspectiveProjectionBase {
        T fov;
        T width;
        T height;
        T nearPlane;
        std::optional<T> farPlane;
    };

    template <typename T>
    struct ReversedZOrthogonalProjection : ReversedZOrthogonalProjectionBase<T> {
        ReversedZOrthogonalProjection();
        ReversedZOrthogonalProjection(T inWidth, T inHeight, T inNearPlane, const std::optional<T>& inFarPlane = {});

        Mat<T, 4, 4> GetProjectionMatrix() const;
        bool operator==(const ReversedZOrthogonalProjection& inRhs) const;
    };

    template <typename T>
    struct ReversedZPerspectiveProjection : ReversedZPerspectiveProjectionBase<T> {
        ReversedZPerspectiveProjection();
        ReversedZPerspectiveProjection(T inFOV, T inWidth, T inHeight, T inNearPlane, const std::optional<T>& inFarPlane);

        Mat<T, 4, 4> GetProjectionMatrix() const;
        bool operator==(const ReversedZPerspectiveProjection& inRhs) const;
    };

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const ReversedZOrthogonalProjection<T>& lhs, const ReversedZOrthogonalProjection<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const ReversedZPerspectiveProjection<T>& lhs, const ReversedZPerspectiveProjection<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    using HReversedZOrthoProjection = ReversedZOrthogonalProjection<HFloat>;
    using FReversedZOrthoProjection = ReversedZOrthogonalProjection<float>;
    using DReversedZOrthoProjection = ReversedZOrthogonalProjection<double>;

    using HReversedZPerspectiveProjection = ReversedZPerspectiveProjection<HFloat>;
    using FReversedZPerspectiveProjection = ReversedZPerspectiveProjection<float>;
    using DReversedZPerspectiveProjection = ReversedZPerspectiveProjection<double>;
}

namespace Common {
    template <typename T>
    ReversedZOrthogonalProjection<T>::ReversedZOrthogonalProjection()
    {
        this->width = 0;
        this->height = 0;
        this->nearPlane = 0;
        this->farPlane = {};
    }

    template <typename T>
    ReversedZOrthogonalProjection<T>::ReversedZOrthogonalProjection(T inWidth, T inHeight, T inNearPlane, const std::optional<T>& inFarPlane)
    {
        this->width = inWidth;
        this->height = inHeight;
        this->nearPlane = inNearPlane;
        this->farPlane = inFarPlane;
    }

    template <typename T>
    Mat<T, 4, 4> ReversedZOrthogonalProjection<T>::GetProjectionMatrix() const
    {
        if (this->farPlane.has_value()) {
            return Mat<T, 4, 4>(
                2.0f / this->width, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f / this->height, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f / (this->farPlane.value() - this->nearPlane), 1.0f + (this->nearPlane / (this->farPlane.value() - this->nearPlane)),
                0.0f, 0.0f, 0.0f, 1.0f
            );
        } else {
            // Infinite Far Plane
            return Mat<T, 4, 4>(
                2.0f / this->width, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f / this->height, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );
        }
    }

    template <typename T>
    bool ReversedZOrthogonalProjection<T>::operator==(const ReversedZOrthogonalProjection& inRhs) const
    {
        const auto thisHasFar = this->farPlane.has_value();
        const auto rhsHasFar = inRhs.farPlane.has_value();
        if (thisHasFar && !rhsHasFar || !thisHasFar && rhsHasFar) {
            return false;
        }
        if (thisHasFar && this->farPlane.value() != inRhs.farPlane.value()) {
            return false;
        }

        return this->width == inRhs.width
            && this->height == inRhs.height
            && this->nearPlane == inRhs.nearPlane;
    }

    template <typename T>
    ReversedZPerspectiveProjection<T>::ReversedZPerspectiveProjection()
    {
        this->fov = 0.0f;
        this->width = 0.0f;
        this->height = 0.0f;
        this->nearPlane = 0;
        this->farPlane = {};
    }

    template <typename T>
    ReversedZPerspectiveProjection<T>::ReversedZPerspectiveProjection(T inFOV, T inWidth, T inHeight, T inNearPlane, const std::optional<T>& inFarPlane)
    {
        this->fov = inFOV;
        this->width = inWidth;
        this->height = inHeight;
        this->nearPlane = inNearPlane;
        this->farPlane = inFarPlane;
    }

    template <typename T>
    Mat<T, 4, 4> ReversedZPerspectiveProjection<T>::GetProjectionMatrix() const
    {
        Angle<T> angle(this->fov);
        T tanHalfFov = tan(angle.ToRadian() / static_cast<T>(2));

        if (this->farPlane.has_value()) {
            return Mat<T, 4, 4>(
                this->height / (this->width * tanHalfFov), 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f / tanHalfFov, 0.0f, 0.0f,
                0.0f, 0.0f, this->nearPlane / (this->nearPlane - this->farPlane.value()), this->nearPlane * this->farPlane.value() / (this->farPlane.value() - this->nearPlane),
                0.0f, 0.0f, 1.0f, 0.0f
            );
        }

        // Infinite Far Plane
        return Mat<T, 4, 4>(
            this->height / (this->width * tanHalfFov), 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f / tanHalfFov, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, this->nearPlane,
            0.0f, 0.0f, 1.0f, 0.0f
        );
    }

    template <typename T>
    bool ReversedZPerspectiveProjection<T>::operator==(const ReversedZPerspectiveProjection& inRhs) const
    {
        const auto thisHasFar = this->farPlane.has_value();
        const auto rhsHasFar = inRhs.farPlane.has_value();
        if (thisHasFar && !rhsHasFar || !thisHasFar && rhsHasFar) {
            return false;
        }
        if (thisHasFar && this->farPlane.value() != inRhs.farPlane.value()) {
            return false;
        }

        return this->fov == inRhs.fov
            && this->width == inRhs.width
            && this->height == inRhs.height
            && this->nearPlane == inRhs.nearPlane;
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const ReversedZOrthogonalProjection<T>& lhs, const ReversedZOrthogonalProjection<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        if (lhs.farPlane.has_value() != rhs.farPlane.has_value()) {
            return false;
        }
        return (!lhs.farPlane.has_value() || AlmostEqual(lhs.farPlane.value(), rhs.farPlane.value(), absoluteTolerance, relativeTolerance))
            && AlmostEqual(lhs.width, rhs.width, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.height, rhs.height, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.nearPlane, rhs.nearPlane, absoluteTolerance, relativeTolerance);
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const ReversedZPerspectiveProjection<T>& lhs, const ReversedZPerspectiveProjection<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        if (lhs.farPlane.has_value() != rhs.farPlane.has_value()) {
            return false;
        }
        return (!lhs.farPlane.has_value() || AlmostEqual(lhs.farPlane.value(), rhs.farPlane.value(), absoluteTolerance, relativeTolerance))
            && AlmostEqual(lhs.fov, rhs.fov, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.width, rhs.width, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.height, rhs.height, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.nearPlane, rhs.nearPlane, absoluteTolerance, relativeTolerance);
    }
}

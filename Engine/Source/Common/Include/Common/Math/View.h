//
// Created by johnk on 2023/7/7.
//

#pragma once

#include <Common/Math/Transform.h>

namespace Common {
    template <typename T>
    struct ViewTransform : Transform<T> {
        static ViewTransform LookAt(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ);

        ViewTransform();
        ViewTransform(Quaternion<T> inRotation, Vec<T, 3> inTranslation);
        explicit ViewTransform(const Transform<T>& inTransform);
        ViewTransform(const ViewTransform& inOther);
        ViewTransform(ViewTransform&& inOther) noexcept;
        ViewTransform& operator=(const ViewTransform& inOther);
        bool operator==(const ViewTransform& inRhs) const;

        Mat<T, 4, 4> GetViewMatrix() const;
    };

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const ViewTransform<T>& lhs, const ViewTransform<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    using HViewTransform = ViewTransform<HFloat>;
    using FViewTransform = ViewTransform<float>;
    using DViewTransform = ViewTransform<double>;
}

namespace Common {
    template <typename T>
    ViewTransform<T> ViewTransform<T>::LookAt(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection)
    {
        return ViewTransform<T>(Transform<T>::LookAt(inPosition, inTargetPosition, inUpDirection));
    }

    template <typename T>
    ViewTransform<T>::ViewTransform()
        : Transform<T>()
    {
    }

    template <typename T>
    ViewTransform<T>::ViewTransform(Quaternion<T> inRotation, Vec<T, 3> inTranslation)
        : Transform<T>(std::move(inRotation), std::move(inTranslation))
    {
    }

    template <typename T>
    ViewTransform<T>::ViewTransform(const Transform<T>& inTransform)
        : Transform<T>(inTransform)
    {
    }

    template <typename T>
    ViewTransform<T>::ViewTransform(const ViewTransform& inOther) = default;

    template <typename T>
    ViewTransform<T>::ViewTransform(ViewTransform&& inOther) noexcept = default;

    template <typename T>
    ViewTransform<T>& ViewTransform<T>::operator=(const ViewTransform& inOther) = default;

    template <typename T>
    bool ViewTransform<T>::operator==(const ViewTransform& inRhs) const
    {
        return Transform<T>::operator==(inRhs);
    }

    template <typename T>
    Mat<T, 4, 4> ViewTransform<T>::GetViewMatrix() const
    {
        // before apply axis transform Mat:
        //     x+ -> from screen outer to inner
        //     y+ -> from left to right
        //     z+ -> from bottom to top
        // after apply axis transform Mat:
        //     x+ -> from left to right
        //     y+ -> from bottom to top
        //     z+ -> from screen outer to inner
        const Mat<T, 4, 4> rotation = this->GetRotationMatrix();
        const T tx = this->translation.x;
        const T ty = this->translation.y;
        const T tz = this->translation.z;
        return Mat<T, 4, 4>(
            rotation.At(0, 1), rotation.At(1, 1), rotation.At(2, 1), -(rotation.At(0, 1) * tx + rotation.At(1, 1) * ty + rotation.At(2, 1) * tz),
            rotation.At(0, 2), rotation.At(1, 2), rotation.At(2, 2), -(rotation.At(0, 2) * tx + rotation.At(1, 2) * ty + rotation.At(2, 2) * tz),
            rotation.At(0, 0), rotation.At(1, 0), rotation.At(2, 0), -(rotation.At(0, 0) * tx + rotation.At(1, 0) * ty + rotation.At(2, 0) * tz),
            0, 0, 0, 1);
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const ViewTransform<T>& lhs, const ViewTransform<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(static_cast<const Transform<T>&>(lhs), static_cast<const Transform<T>&>(rhs), absoluteTolerance, relativeTolerance);
    }
}

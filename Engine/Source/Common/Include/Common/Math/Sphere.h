//
// Created by johnk on 2023/7/8.
//

#pragma once

#include <Common/Math/Vector.h>

namespace Common {
    template <typename T>
    struct SphereBase {
        Vec<T, 3> center;
        T radius;
    };

    template <typename T>
    struct Sphere : SphereBase<T> {
        Sphere();
        Sphere(T inX, T inY, T inZ, T inRadius);
        Sphere(Vec<T, 3> inCenter, T inRadius);
        Sphere(const Sphere& inOther);
        Sphere(Sphere&& inOther) noexcept;
        Sphere& operator=(const Sphere& inOther);

        T Distance(const Sphere& inOther) const requires FloatingPoint<T>;
        bool Inside(const Vec<T, 3>& inPoint) const;
        bool Intersect(const Sphere& inOther) const;
        bool operator==(const Sphere& inRhs) const;

        template <typename IT> Sphere<IT> CastTo() const;
    };

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const Sphere<T>& lhs, const Sphere<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    using ISphere = Sphere<int32_t>;
    using HSphere = Sphere<HFloat>;
    using FSphere = Sphere<float>;
    using DSphere = Sphere<double>;
}

namespace Common {
    template <typename T>
    Sphere<T>::Sphere()
    {
        this->center = VecConsts<T, 3>::zero;
        this->radius = 0;
    }

    template <typename T>
    Sphere<T>::Sphere(T inX, T inY, T inZ, T inRadius)
    {
        this->center = Vec<T, 3>(inX, inY, inZ);
        this->radius = inRadius;
    }

    template <typename T>
    Sphere<T>::Sphere(Vec<T, 3> inCenter, T inRadius)
    {
        this->center = std::move(inCenter);
        this->radius = inRadius;
    }

    template <typename T>
    Sphere<T>::Sphere(const Sphere<T>& inOther) = default;

    template <typename T>
    Sphere<T>::Sphere(Sphere<T>&& inOther) noexcept = default;

    template <typename T>
    Sphere<T>& Sphere<T>::operator=(const Sphere<T>& inOther) = default;

    template <typename T>
    T Sphere<T>::Distance(const Sphere& inOther) const requires FloatingPoint<T>
    {
        Vec<T, 3> direction = this->center - inOther.center;
        return direction.Model();
    }

    template <typename T>
    bool Sphere<T>::Inside(const Vec<T, 3>& inPoint) const
    {
        if (this->radius < static_cast<T>(0)) {
            return false;
        }
        const Vec<T, 3> direction = this->center - inPoint;
        return direction.ModelSquared() <= this->radius * this->radius;
    }

    template <typename T>
    bool Sphere<T>::Intersect(const Sphere& inOther) const
    {
        const T combinedRadius = this->radius + inOther.radius;
        if (combinedRadius < static_cast<T>(0)) {
            return false;
        }
        const Vec<T, 3> direction = this->center - inOther.center;
        return direction.ModelSquared() <= combinedRadius * combinedRadius;
    }

    template <typename T>
    bool Sphere<T>::operator==(const Sphere& inRhs) const
    {
        return this->center == inRhs.center
            && this->radius == inRhs.radius;
    }

    template <typename T>
    template <typename IT>
    Sphere<IT> Sphere<T>::CastTo() const
    {
        return Sphere<IT>(
            this->center.template CastTo<IT>(),
            static_cast<IT>(this->radius)
        );
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const Sphere<T>& lhs, const Sphere<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(lhs.center, rhs.center, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.radius, rhs.radius, absoluteTolerance, relativeTolerance);
    }
}

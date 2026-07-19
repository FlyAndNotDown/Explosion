//
// Created by johnk on 2023/6/4.
//

#pragma once

#include <algorithm>

#include <Common/Math/Simd.h>
#include <Common/Math/Half.h>
#include <Common/Math/Matrix.h>

namespace Common {
    template <typename T> struct Angle;
    template <typename T> struct Radian;

    template <FloatingPoint T>
    struct AngleBase {
        T value;
    };

    template <FloatingPoint T>
    struct RadianBase {
        T value;
    };

    template <FloatingPoint T>
    struct QuaternionBase {
        T x;
        T y;
        T z;
        T w;
    };

    template <typename T>
    struct Angle : AngleBase<T> {
        Angle();
        explicit Angle(T inValue);
        explicit Angle(const Radian<T>& inValue);
        Angle(const Angle& inValue);
        Angle(Angle&& inValue) noexcept;
        Angle& operator=(const Angle& inValue);
        Angle& operator=(const Radian<T>& inValue);
        bool operator==(const Angle& inRhs) const;
        T ToRadian() const;
    };

    template <typename T>
    struct Radian : RadianBase<T> {
        Radian();
        explicit Radian(T inValue);
        explicit Radian(const Angle<T>& inValue);
        Radian(const Radian& inValue);
        Radian(Radian&& inValue) noexcept;
        Radian& operator=(const Radian& inValue);
        Radian& operator=(const Angle<T>& inValue);
        bool operator==(const Radian& inRhs) const;
        T ToAngle() const;
    };

    // left-hand coordinates system
    // +x -> from screen outer to inner
    // +y -> from left to right
    // +z -> from bttom to up
    template <typename T, MathBackend B = MathBackend::defaultBackend>
    struct Quaternion : QuaternionBase<T> {
        static Quaternion FromEulerZYX(T inAngleX, T inAngleY, T inAngleZ);
        static Quaternion FromEulerZYX(const Radian<T>& inRadianX, const Radian<T>& inRadianY, const Radian<T>& inRadianZ);

        Quaternion();
        Quaternion(T inW, T inX, T inY, T inZ);
        Quaternion(const Vec<T, 3, B>& inAxis, T inAngle);
        Quaternion(const Vec<T, 3, B>& inAxis, const Radian<T>& inRadian);
        Quaternion(const Quaternion& inValue) = default;
        Quaternion(Quaternion&& inValue) noexcept = default;
        Quaternion& operator=(const Quaternion& inValue) = default;

        bool operator==(const Quaternion& rhs) const;
        bool operator!=(const Quaternion& rhs) const;
        Quaternion operator+(const Quaternion& rhs) const;
        Quaternion operator-(const Quaternion& rhs) const;
        Quaternion operator*(T rhs) const;
        Quaternion operator*(const Quaternion& rhs) const;
        Quaternion operator/(T rhs) const;

        Quaternion& operator+=(const Quaternion& rhs);
        Quaternion& operator-=(const Quaternion& rhs);
        Quaternion& operator*=(T rhs);
        Quaternion& operator*=(const Quaternion& rhs);
        Quaternion& operator/=(T rhs);

        Vec<T, 3, B> ImaginaryPart() const;
        T ModelSquared() const;
        T Model() const;
        bool IsNormalized(T tolerance = DefaultTolerance<T>()) const;
        Quaternion Negatived() const;
        Quaternion Conjugated() const;
        Quaternion Normalized() const;
        bool TryNormalize(T tolerance = DefaultTolerance<T>());
        void Normalize();
        T Dot(const Quaternion& rhs) const;
        // when axis faced to us, ccw as positive direction
        Vec<T, 3, B> RotateVector(const Vec<T, 3, B>& inVector) const;
        Vec<T, 3, B> ToEulerZYX() const;
        Mat<T, 4, 4, B> GetRotationMatrix() const;

        template <typename IT>
        Quaternion<IT, B> CastTo() const;
    };

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const Angle<T>& lhs, const Angle<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const Radian<T>& lhs, const Radian<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    template <typename T, MathBackend B> requires FloatingPoint<T> bool AlmostEqual(const Quaternion<T, B>& lhs, const Quaternion<T, B>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    template <typename T, MathBackend B = MathBackend::defaultBackend>
    struct QuatConsts {
        static const Quaternion<T, B> zero;
        static const Quaternion<T, B> identity;
    };

    using HAngle = Angle<HFloat>;
    using FAngle = Angle<float>;
    using DAngle = Angle<double>;

    using HRadian = Radian<HFloat>;
    using FRadian = Radian<float>;
    using DRadian = Radian<double>;

    using HQuat = Quaternion<HFloat>;
    using FQuat = Quaternion<float>;
    using DQuat = Quaternion<double>;

    using HQuatConsts = QuatConsts<HFloat>;
    using FQuatConsts = QuatConsts<float>;
    using DQuatConsts = QuatConsts<double>;
}

namespace Common::Internal {
    // Per-backend dispatch for quaternion arithmetic. The primary template is the scalar implementation, so any
    // (T, B) without a specialization degrades gracefully to scalar; the SIMD specialization follows immediately after.
    template <typename T, MathBackend B>
    struct QuatOps {
        using Q = Quaternion<T, B>;

        static Q Add(const Q& a, const Q& b)
        {
            Q result;
            result.w = a.w + b.w;
            result.x = a.x + b.x;
            result.y = a.y + b.y;
            result.z = a.z + b.z;
            return result;
        }

        static Q Sub(const Q& a, const Q& b)
        {
            Q result;
            result.w = a.w - b.w;
            result.x = a.x - b.x;
            result.y = a.y - b.y;
            result.z = a.z - b.z;
            return result;
        }

        static Q MulScalar(const Q& a, T b)
        {
            Q result;
            result.w = a.w * b;
            result.x = a.x * b;
            result.y = a.y * b;
            result.z = a.z * b;
            return result;
        }

        static Q DivScalar(const Q& a, T b)
        {
            Q result;
            result.w = a.w / b;
            result.x = a.x / b;
            result.y = a.y / b;
            result.z = a.z / b;
            return result;
        }

        static Q Mul(const Q& a, const Q& b)
        {
            Q result;
            result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
            result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
            result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
            result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
            return result;
        }

        static T Dot(const Q& a, const Q& b)
        {
            return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
        }
    };

    // QuaternionBase<float> stores x, y, z, w as four contiguous floats, so &q.x is the base of a 16-byte block that
    // maps to an unaligned 128-bit load/store. The element-wise ops and the dot product map onto the F32x4 wrapper
    // directly; the Hamilton product is expressed as four broadcast-and-permute terms (see Mul).
    template <>
    struct QuatOps<float, MathBackend::simd> {
        using Q = Quaternion<float, MathBackend::simd>;

        static Q Add(const Q& a, const Q& b) { Q r; Simd::MapBinary<4>(&r.x, &a.x, &b.x, Simd::AddOp {}); return r; }
        static Q Sub(const Q& a, const Q& b) { Q r; Simd::MapBinary<4>(&r.x, &a.x, &b.x, Simd::SubOp {}); return r; }

        static Q MulScalar(const Q& a, float b) { Q r; Simd::MapScalar<4>(&r.x, &a.x, b, Simd::MulOp {}); return r; }
        static Q DivScalar(const Q& a, float b) { Q r; Simd::MapScalar<4>(&r.x, &a.x, b, Simd::DivOp {}); return r; }

        // Hamilton product, with both quaternions loaded as (x, y, z, w). Each row of the product is one component of
        // a broadcast against a sign-flipped permutation of b, summed across the four components of a:
        //   result = aw*(bx,by,bz,bw) + ax*(bw,-bz,by,-bx) + ay*(bz,bw,-bx,-by) + az*(-by,bx,bw,-bz)
        // The accumulation order matches the scalar reference above, so both backends produce identical results.
        static Q Mul(const Q& a, const Q& b)
        {
            const Simd::F32x4 av = Simd::LoadU(&a.x);
            const Simd::F32x4 bv = Simd::LoadU(&b.x);

            const Simd::F32x4 sign0 = Simd::Set(1.0f, -1.0f, 1.0f, -1.0f);
            const Simd::F32x4 sign1 = Simd::Set(1.0f, 1.0f, -1.0f, -1.0f);
            const Simd::F32x4 sign2 = Simd::Set(-1.0f, 1.0f, 1.0f, -1.0f);

            Simd::F32x4 acc = Simd::Mul(Simd::Splat<3>(av), bv);
            acc = Simd::Add(acc, Simd::Mul(Simd::Splat<0>(av), Simd::Mul(Simd::Shuffle<3, 2, 1, 0>(bv), sign0)));
            acc = Simd::Add(acc, Simd::Mul(Simd::Splat<1>(av), Simd::Mul(Simd::Shuffle<2, 3, 0, 1>(bv), sign1)));
            acc = Simd::Add(acc, Simd::Mul(Simd::Splat<2>(av), Simd::Mul(Simd::Shuffle<1, 0, 3, 2>(bv), sign2)));

            Q result;
            Simd::StoreU(&result.x, acc);
            return result;
        }

        static float Dot(const Q& a, const Q& b)
        {
            return Simd::Sum(Simd::Mul(Simd::LoadU(&a.x), Simd::LoadU(&b.x)));
        }
    };
}

namespace Common {
    template <typename T>
    Angle<T>::Angle()
    {
        this->value = 0;
    }

    template <typename T>
    Angle<T>::Angle(T inValue)
    {
        this->value = inValue;
    }

    template <typename T>
    Angle<T>::Angle(const Radian<T>& inValue) : Angle(inValue.ToAngle()) {}

    template <typename T>
    Angle<T>::Angle(const Angle& inValue) = default;

    template <typename T>
    Angle<T>::Angle(Angle&& inValue) noexcept = default;

    template <typename T>
    Angle<T>& Angle<T>::operator=(const Angle& inValue) = default;

    template <typename T>
    Angle<T>& Angle<T>::operator=(const Radian<T>& inValue)
    {
        this->value = inValue.ToAngle();
        return *this;
    }

    template <typename T>
    bool Angle<T>::operator==(const Angle& inRhs) const
    {
        return this->value == inRhs.value;
    }

    template <typename T>
    T Angle<T>::ToRadian() const
    {
        return this->value / static_cast<T>(180) * Pi<T>();
    }

    template <typename T>
    Radian<T>::Radian()
    {
        this->value = 0;
    }

    template <typename T>
    Radian<T>::Radian(T inValue)
    {
        this->value = inValue;
    }

    template <typename T>
    Radian<T>::Radian(const Angle<T>& inValue) : Radian(inValue.ToRadian()) {}

    template <typename T>
    Radian<T>::Radian(const Radian& inValue) = default;

    template <typename T>
    Radian<T>::Radian(Radian&& inValue) noexcept = default;

    template <typename T>
    Radian<T>& Radian<T>::operator=(const Radian& inValue) = default;

    template <typename T>
    Radian<T>& Radian<T>::operator=(const Angle<T>& inValue)
    {
        this->value = inValue.ToRadian();
        return *this;
    }

    template <typename T>
    bool Radian<T>::operator==(const Radian& inRhs) const
    {
        return this->value == inRhs.value;
    }

    template <typename T>
    T Radian<T>::ToAngle() const
    {
        return this->value * static_cast<T>(180) / Pi<T>();
    }

    template <typename T, MathBackend B>
    const Quaternion<T, B> QuatConsts<T, B>::zero = Quaternion<T, B>();

    template <typename T, MathBackend B>
    const Quaternion<T, B> QuatConsts<T, B>::identity = Quaternion<T, B>(1, 0, 0, 0);

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::FromEulerZYX(T inAngleX, T inAngleY, T inAngleZ)
    {
        return Quaternion(VecConsts<T, 3, B>::unitZ, inAngleZ)
            * Quaternion(VecConsts<T, 3, B>::unitY, inAngleY)
            * Quaternion(VecConsts<T, 3, B>::unitX, inAngleX);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::FromEulerZYX(const Radian<T>& inRadianX, const Radian<T>& inRadianY, const Radian<T>& inRadianZ)
    {
        return FromEulerZYX(inRadianX.ToAngle(), inRadianY.ToAngle(), inRadianZ.ToAngle());
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>::Quaternion()
    {
        this->w = 0;
        this->x = 0;
        this->y = 0;
        this->z = 0;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>::Quaternion(T inW, T inX, T inY, T inZ)
    {
        this->w = inW;
        this->x = inX;
        this->y = inY;
        this->z = inZ;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>::Quaternion(const Vec<T, 3, B>& inAxis, T inAngle)
    {
        Vec<T, 3, B> axis = inAxis;
        if (!axis.TryNormalize()) {
            Assert(false);
            this->w = 1;
            this->x = 0;
            this->y = 0;
            this->z = 0;
            return;
        }
        const T halfRadian = Angle<T>(inAngle).ToRadian() / static_cast<T>(2);
        const T halfRadianSin = std::sin(halfRadian);
        const T halfRadianCos = std::cos(halfRadian);

        this->w = halfRadianCos;
        this->x = axis.x * halfRadianSin;
        this->y = axis.y * halfRadianSin;
        this->z = axis.z * halfRadianSin;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>::Quaternion(const Vec<T, 3, B>& inAxis, const Radian<T>& inRadian)
        : Quaternion(inAxis, inRadian.ToAngle())
    {
    }

    template <typename T, MathBackend B>
    bool Quaternion<T, B>::operator==(const Quaternion& rhs) const
    {
        return this->w == rhs.w
            && this->x == rhs.x
            && this->y == rhs.y
            && this->z == rhs.z;
    }

    template <typename T, MathBackend B>
    bool Quaternion<T, B>::operator!=(const Quaternion& rhs) const
    {
        return !this->operator==(rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::operator+(const Quaternion& rhs) const
    {
        return Internal::QuatOps<T, B>::Add(*this, rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::operator-(const Quaternion& rhs) const
    {
        return Internal::QuatOps<T, B>::Sub(*this, rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::operator*(T rhs) const
    {
        return Internal::QuatOps<T, B>::MulScalar(*this, rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::operator*(const Quaternion& rhs) const
    {
        return Internal::QuatOps<T, B>::Mul(*this, rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::operator/(T rhs) const
    {
        return Internal::QuatOps<T, B>::DivScalar(*this, rhs);
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>& Quaternion<T, B>::operator+=(const Quaternion& rhs)
    {
        *this = Internal::QuatOps<T, B>::Add(*this, rhs);
        return *this;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>& Quaternion<T, B>::operator-=(const Quaternion& rhs)
    {
        *this = Internal::QuatOps<T, B>::Sub(*this, rhs);
        return *this;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>& Quaternion<T, B>::operator*=(T rhs)
    {
        *this = Internal::QuatOps<T, B>::MulScalar(*this, rhs);
        return *this;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>& Quaternion<T, B>::operator*=(const Quaternion& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B>& Quaternion<T, B>::operator/=(T rhs)
    {
        *this = Internal::QuatOps<T, B>::DivScalar(*this, rhs);
        return *this;
    }

    template <typename T, MathBackend B>
    Vec<T, 3, B> Quaternion<T, B>::ImaginaryPart() const
    {
        return Vec<T, 3, B>(this->x, this->y, this->z);
    }

    template <typename T, MathBackend B>
    T Quaternion<T, B>::Model() const
    {
        return std::sqrt(ModelSquared());
    }

    template <typename T, MathBackend B>
    T Quaternion<T, B>::ModelSquared() const
    {
        return Internal::QuatOps<T, B>::Dot(*this, *this);
    }

    template <typename T, MathBackend B>
    bool Quaternion<T, B>::IsNormalized(T tolerance) const
    {
        using EvaluationT = std::conditional_t<HalfFloatingPoint<T>, float, T>;
        const EvaluationT modelSquared = static_cast<EvaluationT>(ModelSquared());
        const EvaluationT toleranceValue = std::abs(static_cast<EvaluationT>(tolerance));
        return std::isfinite(modelSquared) && std::abs(modelSquared - static_cast<EvaluationT>(1)) <= toleranceValue;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::Negatived() const
    {
        Quaternion result;
        result.w = -this->w;
        result.x = -this->x;
        result.y = -this->y;
        result.z = -this->z;
        return result;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::Conjugated() const
    {
        Quaternion result;
        result.w = this->w;
        result.x = -this->x;
        result.y = -this->y;
        result.z = -this->z;
        return result;
    }

    template <typename T, MathBackend B>
    Quaternion<T, B> Quaternion<T, B>::Normalized() const
    {
        Quaternion result(*this);
        result.Normalize();
        return result;
    }

    template <typename T, MathBackend B>
    bool Quaternion<T, B>::TryNormalize(T tolerance)
    {
        const T modelSquared = ModelSquared();
        const double modelSquaredValue = static_cast<double>(modelSquared);
        const double toleranceValue = static_cast<double>(tolerance);
        if (!std::isfinite(modelSquaredValue) || modelSquaredValue <= toleranceValue * toleranceValue) {
            return false;
        }

        const T oneOverModel = static_cast<T>(1) / static_cast<T>(std::sqrt(modelSquared));
        *this = Internal::QuatOps<T, B>::MulScalar(*this, oneOverModel);
        return true;
    }

    template <typename T, MathBackend B>
    void Quaternion<T, B>::Normalize()
    {
        const bool normalized = TryNormalize();
        Assert(normalized);
    }

    template <typename T, MathBackend B>
    T Quaternion<T, B>::Dot(const Quaternion& rhs) const
    {
        return Internal::QuatOps<T, B>::Dot(*this, rhs);
    }

    template <typename T, MathBackend B>
    Vec<T, 3, B> Quaternion<T, B>::RotateVector(const Vec<T, 3, B>& inVector) const
    {
        const Vec<T, 3, B> imaginary = ImaginaryPart();
        const Vec<T, 3, B> twiceCross = inVector.Cross(imaginary) * static_cast<T>(2);
        return inVector + twiceCross * this->w + twiceCross.Cross(imaginary);
    }

    template <typename T, MathBackend B>
    Vec<T, 3, B> Quaternion<T, B>::ToEulerZYX() const
    {
        const T normSquared = Dot(*this);
        if (normSquared == static_cast<T>(0)) {
            return VecConsts<T, 3, B>::zero;
        }

        const T sinY = std::clamp(T(2.0f) * (this->w * this->y - this->z * this->x) / normSquared, T(-1.0f), T(1.0f));
        const Radian<T> radianX(static_cast<T>(std::atan2(
            T(2.0f) * (this->w * this->x + this->y * this->z),
            normSquared - T(2.0f) * (this->x * this->x + this->y * this->y))));
        const Radian<T> radianY(static_cast<T>(std::asin(sinY)));
        const Radian<T> radianZ(static_cast<T>(std::atan2(
            T(2.0f) * (this->w * this->z + this->x * this->y),
            normSquared - T(2.0f) * (this->y * this->y + this->z * this->z))));
        return Vec<T, 3, B>(radianX.ToAngle(), radianY.ToAngle(), radianZ.ToAngle());
    }

    template <typename T, MathBackend B>
    Mat<T, 4, 4, B> Quaternion<T, B>::GetRotationMatrix() const
    {
        T xx2 = this->x * this->x * 2;
        T yy2 = this->y * this->y * 2;
        T zz2 = this->z * this->z * 2;

        T wx2 = this->w * this->x * 2;
        T wy2 = this->w * this->y * 2;
        T wz2 = this->w * this->z * 2;
        T xy2 = this->x * this->y * 2;
        T xz2 = this->x * this->z * 2;
        T yz2 = this->y * this->z * 2;

        return Mat<T, 4, 4, B>(
            1 - yy2 - zz2, xy2 + wz2, xz2 - wy2, 0,
            xy2 - wz2, 1 - xx2 - zz2, yz2 + wx2, 0,
            xz2 + wy2, yz2 - wx2, 1 - xx2 - yy2, 0,
            0, 0, 0, 1
        );
    }

    template <typename T, MathBackend B>
    template <typename IT>
    Quaternion<IT, B> Quaternion<T, B>::CastTo() const
    {
        Quaternion<IT, B> result;
        result.w = static_cast<IT>(this->w);
        result.x = static_cast<IT>(this->x);
        result.y = static_cast<IT>(this->y);
        result.z = static_cast<IT>(this->z);
        return result;
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const Angle<T>& lhs, const Angle<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(lhs.value, rhs.value, absoluteTolerance, relativeTolerance);
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const Radian<T>& lhs, const Radian<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(lhs.value, rhs.value, absoluteTolerance, relativeTolerance);
    }

    template <typename T, MathBackend B>
    requires FloatingPoint<T>
    bool AlmostEqual(const Quaternion<T, B>& lhs, const Quaternion<T, B>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(lhs.w, rhs.w, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.x, rhs.x, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.y, rhs.y, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.z, rhs.z, absoluteTolerance, relativeTolerance);
    }
}

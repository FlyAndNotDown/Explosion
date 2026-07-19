//
// Created by johnk on 2023/6/21.
//

#pragma once

#include <Common/Math/Vector.h>
#include <Common/Math/Matrix.h>
#include <Common/Math/Quaternion.h>

namespace Common {
    template <FloatingPoint T>
    struct TransformBase {
        Vec<T, 3> scale;
        Quaternion<T> rotation;
        Vec<T, 3> translation;
    };

    template <typename T>
    struct Transform : TransformBase<T> {
        static Transform LookAt(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ);
        static bool TryFromMatrix(const Mat<T, 4, 4>& inMatrix, Transform& outTransform, T tolerance = DefaultTolerance<T>());

        Transform();
        Transform(const Mat<T, 4, 4>& inMatrix);
        Transform(Quaternion<T> inRotation, Vec<T, 3> inTranslation);
        Transform(Vec<T, 3> inScale, Quaternion<T> inRotation, Vec<T, 3> inTranslation);
        Transform(const Transform& other);
        Transform(Transform&& other) noexcept;
        Transform& operator=(const Transform& other);

        bool operator==(const Transform& rhs) const;
        bool operator!=(const Transform& rhs) const;

        Transform operator+(const Vec<T, 3>& inTranslation) const;
        Transform operator|(const Quaternion<T>& inRotation) const;
        Transform operator*(const Vec<T, 3>& inScale) const;

        Transform& operator+=(const Vec<T, 3>& inTranslation);
        Transform& operator|=(const Quaternion<T>& inRotation);
        Transform& operator*=(const Vec<T, 3>& inScale);

        Transform& Translate(const Vec<T, 3>& inTranslation);
        Transform& Rotate(const Quaternion<T>& inRotation);
        Transform& Scale(const Vec<T, 3>& inScale);
        bool TrySetFromMatrix(const Mat<T, 4, 4>& inMatrix, T tolerance = DefaultTolerance<T>());
        Transform& UpdateRotation(const Vec<T, 3>& forward, const Vec<T, 3>& side, const Vec<T, 3>& up);
        bool TryLookTo(const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ, T tolerance = DefaultTolerance<T>());
        bool TryMoveAndLookTo(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ, T tolerance = DefaultTolerance<T>());
        Transform& LookTo(const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ);
        Transform& MoveAndLookTo(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection = VecConsts<T, 3>::unitZ);

        Mat<T, 4, 4> GetTranslationMatrix() const;
        Mat<T, 4, 4> GetRotationMatrix() const;
        Mat<T, 4, 4> GetScaleMatrix() const;
        // scale -> rotate -> translate
        Mat<T, 4, 4> GetTransformMatrix() const;
        // rotate -> translate
        Mat<T, 4, 4> GetTransformMatrixNoScale() const;
        Vec<T, 3> TransformPosition(const Vec<T, 3>& inPosition) const;
        Vec<T, 4> TransformPosition(const Vec<T, 4>& inPosition) const;

        template <typename IT>
        Transform<IT> CastTo() const;
    };

    template <typename T> requires FloatingPoint<T> bool AlmostEqual(const Transform<T>& lhs, const Transform<T>& rhs, T absoluteTolerance = DefaultTolerance<T>(), T relativeTolerance = DefaultTolerance<T>());

    using HTransform = Transform<HFloat>;
    using FTransform = Transform<float>;
    using DTransform = Transform<double>;
}

namespace Common {
    template <typename T>
    Transform<T> Transform<T>::LookAt(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection)
    {
        Transform result;
        result.MoveAndLookTo(inPosition, inTargetPosition, inUpDirection);
        return result;
    }

    template <typename T>
    bool Transform<T>::TryFromMatrix(const Mat<T, 4, 4>& inMatrix, Transform& outTransform, T tolerance)
    {
        Transform result;
        if (!result.TrySetFromMatrix(inMatrix, tolerance)) {
            return false;
        }
        outTransform = result;
        return true;
    }

    template <typename T>
    Transform<T>::Transform()
    {
        this->scale = VecConsts<T, 3>::unit;
        this->rotation = QuatConsts<T>::identity;
        this->translation = VecConsts<T, 3>::zero;
    }

    template <typename T>
    Transform<T>::Transform(const Mat<T, 4, 4>& inMatrix)
        : Transform()
    {
        const bool decomposed = TrySetFromMatrix(inMatrix);
        Assert(decomposed);
    }

    template <typename T>
    Transform<T>::Transform(Quaternion<T> inRotation, Vec<T, 3> inTranslation)
    {
        this->scale = VecConsts<T, 3>::unit;
        this->rotation = inRotation;
        this->translation = inTranslation;
    }

    template <typename T>
    Transform<T>::Transform(Vec<T, 3> inScale, Quaternion<T> inRotation, Vec<T, 3> inTranslation)
    {
        this->scale = inScale;
        this->rotation = inRotation;
        this->translation = inTranslation;
    }

    template <typename T>
    Transform<T>::Transform(const Transform& other) = default;

    template <typename T>
    Transform<T>::Transform(Transform&& other) noexcept = default;

    template <typename T>
    Transform<T>& Transform<T>::operator=(const Transform& other) = default;

    template <typename T>
    bool Transform<T>::operator==(const Transform& rhs) const
    {
        return this->scale == rhs.scale
            && this->rotation == rhs.rotation
            && this->translation == rhs.translation;
    }

    template <typename T>
    bool Transform<T>::operator!=(const Transform& rhs) const
    {
        return !this->operator==(rhs);
    }

    template <typename T>
    Transform<T> Transform<T>::operator+(const Vec<T, 3>& inTranslation) const
    {
        Transform result;
        result.scale = this->scale;
        result.rotation = this->rotation;
        result.translation = this->translation + inTranslation;
        return result;
    }

    template <typename T>
    Transform<T> Transform<T>::operator|(const Quaternion<T>& inRotation) const
    {
        Transform result;
        result.scale = this->scale;
        result.rotation = this->rotation * inRotation;
        result.translation = this->translation;
        return result;
    }

    template <typename T>
    Transform<T> Transform<T>::operator*(const Vec<T, 3>& inScale) const
    {
        Transform result;
        result.scale = this->scale * inScale;
        result.rotation = this->rotation;
        result.translation = this->translation;
        return result;
    }

    template <typename T>
    Transform<T>& Transform<T>::operator+=(const Vec<T, 3>& inTranslation)
    {
        return Translate(inTranslation);
    }

    template <typename T>
    Transform<T>& Transform<T>::operator|=(const Quaternion<T>& inRotation)
    {
        return Rotate(inRotation);
    }

    template <typename T>
    Transform<T>& Transform<T>::operator*=(const Vec<T, 3>& inScale)
    {
        return Scale(inScale);
    }

    template <typename T>
    Transform<T>& Transform<T>::Translate(const Vec<T, 3>& inTranslation)
    {
        this->translation += inTranslation;
        return *this;
    }

    template <typename T>
    Transform<T>& Transform<T>::Rotate(const Quaternion<T>& inRotation)
    {
        this->rotation *= inRotation;
        return *this;
    }

    template <typename T>
    Transform<T>& Transform<T>::Scale(const Vec<T, 3>& inScale)
    {
        this->scale *= inScale;
        return *this;
    }

    template <typename T>
    Transform<T>& Transform<T>::UpdateRotation(const Vec<T, 3>& forward, const Vec<T, 3>& side, const Vec<T, 3>& up)
    {
        // the resulting rotation must map the local axes onto the given frame following the engine convention
        // (local +x -> forward, +y -> side, +z -> up), i.e. GetRotationMatrix() columns must be [forward, side, up];
        // engine quaternions apply as the transpose of the textbook rotation matrix (see
        // Quaternion::GetRotationMatrix), so the textbook extraction below runs on the transposed target, whose
        // rows are forward/side/up
        // Algorithm: https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

        Mat<T, 3, 3> rotMat {
            forward.x, forward.y, forward.z,
            side.x, side.y, side.z,
            up.x, up.y, up.z
        };

        T trace = rotMat.At(0, 0) + rotMat.At(1, 1) + rotMat.At(2, 2);
        if (trace > static_cast<T>(0)) {
            T root = sqrt(trace + static_cast<T>(1.0));
            this->rotation.w = static_cast<T>(0.5) * root;

            root = static_cast<T>(0.5) / root;
            this->rotation.x = root * (rotMat.At(2, 1) - rotMat.At(1, 2));
            this->rotation.y = root * (rotMat.At(0, 2) - rotMat.At(2, 0));
            this->rotation.z = root * (rotMat.At(1, 0) - rotMat.At(0, 1));
        } else if (rotMat.At(0, 0) > rotMat.At(1, 1) && rotMat.At(0, 0) > rotMat.At(2, 2)) {
            T root = sqrt(rotMat.At(0, 0) - rotMat.At(1, 1) - rotMat.At(2, 2) + static_cast<T>(1.0));
            this->rotation.x = static_cast<T>(0.5) * root;

            root = static_cast<T>(0.5) / root;
            this->rotation.y = root * (rotMat.At(0, 1) + rotMat.At(1, 0));
            this->rotation.z = root * (rotMat.At(0, 2) + rotMat.At(2, 0));
            this->rotation.w = root * (rotMat.At(2, 1) - rotMat.At(1, 2));
        } else if (rotMat.At(1, 1) > rotMat.At(2, 2)) {
            T root = sqrt(rotMat.At(1, 1) - rotMat.At(0, 0) - rotMat.At(2, 2) + static_cast<T>(1.0));
            this->rotation.y = static_cast<T>(0.5) * root;

            root = static_cast<T>(0.5) / root;
            this->rotation.x = root * (rotMat.At(0, 1) + rotMat.At(1, 0));
            this->rotation.z = root * (rotMat.At(1, 2) + rotMat.At(2, 1));
            this->rotation.w = root * (rotMat.At(0, 2) - rotMat.At(2, 0));
        } else {
            T root = sqrt(rotMat.At(2, 2) - rotMat.At(0, 0) - rotMat.At(1, 1) + static_cast<T>(1.0));
            this->rotation.z = static_cast<T>(0.5) * root;

            root = static_cast<T>(0.5) / root;
            this->rotation.x = root * (rotMat.At(0, 2) + rotMat.At(2, 0));
            this->rotation.y = root * (rotMat.At(1, 2) + rotMat.At(2, 1));
            this->rotation.w = root * (rotMat.At(1, 0) - rotMat.At(0, 1));
        }

        return *this;
    }

    template <typename T>
    bool Transform<T>::TryLookTo(const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection, T tolerance)
    {
        Vec<T, 3> f(inTargetPosition - this->translation);
        if (!f.TryNormalize(tolerance)) {
            return false;
        }

        Vec<T, 3> s = inUpDirection.Cross(f);
        if (!s.TryNormalize(tolerance)) {
            const Vec<T, 3>& fallbackUp = std::abs(static_cast<double>(f.z)) < 0.999
                ? VecConsts<T, 3>::unitZ
                : VecConsts<T, 3>::unitY;
            s = fallbackUp.Cross(f);
            if (!s.TryNormalize(tolerance)) {
                return false;
            }
        }

        Vec<T, 3> u = f.Cross(s);

        this->UpdateRotation(f, s, u);
        return true;
    }

    template <typename T>
    bool Transform<T>::TryMoveAndLookTo(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection, T tolerance)
    {
        Transform result(*this);
        result.translation = inPosition;
        if (!result.TryLookTo(inTargetPosition, inUpDirection, tolerance)) {
            return false;
        }
        *this = result;
        return true;
    }

    template <typename T>
    Transform<T>& Transform<T>::LookTo(const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection)
    {
        const bool success = TryLookTo(inTargetPosition, inUpDirection);
        Assert(success);
        return *this;
    }

    template <typename T>
    bool Transform<T>::TrySetFromMatrix(const Mat<T, 4, 4>& inMatrix, T tolerance)
    {
        Vec<T, 3> translation;
        Quaternion<T> rotation;
        Vec<T, 3> scale;
        if (!inMatrix.TryDecomposeAffine(translation, rotation, scale, tolerance)) {
            return false;
        }

        this->translation = translation;
        this->rotation = rotation;
        this->scale = scale;
        return true;
    }

    template <typename T>
    Transform<T>& Transform<T>::MoveAndLookTo(const Vec<T, 3>& inPosition, const Vec<T, 3>& inTargetPosition, const Vec<T, 3>& inUpDirection)
    {
        const bool success = TryMoveAndLookTo(inPosition, inTargetPosition, inUpDirection);
        Assert(success);
        return *this;
    }

    template <typename T>
    Mat<T, 4, 4> Transform<T>::GetTranslationMatrix() const
    {
        Mat<T, 4, 4> result = MatConsts<T, 4, 4>::identity;
        result.SetCol(3, this->translation.x, this->translation.y, this->translation.z, 1);
        return result;
    }

    template <typename T>
    Mat<T, 4, 4> Transform<T>::GetRotationMatrix() const
    {
        return this->rotation.GetRotationMatrix();
    }

    template <typename T>
    Mat<T, 4, 4> Transform<T>::GetScaleMatrix() const
    {
        Mat<T, 4, 4> result = MatConsts<T, 4, 4>::identity;
        result.At(0, 0) = this->scale.x;
        result.At(1, 1) = this->scale.y;
        result.At(2, 2) = this->scale.z;
        return result;
    }

    template <typename T>
    Mat<T, 4, 4> Transform<T>::GetTransformMatrix() const
    {
        Mat<T, 4, 4> result = GetRotationMatrix();
        for (auto row = 0; row < 3; row++) {
            result.At(row, 0) *= this->scale.x;
            result.At(row, 1) *= this->scale.y;
            result.At(row, 2) *= this->scale.z;
        }
        result.At(0, 3) = this->translation.x;
        result.At(1, 3) = this->translation.y;
        result.At(2, 3) = this->translation.z;
        return result;
    }

    template <typename T>
    Mat<T, 4, 4> Transform<T>::GetTransformMatrixNoScale() const
    {
        Mat<T, 4, 4> result = GetRotationMatrix();
        result.At(0, 3) = this->translation.x;
        result.At(1, 3) = this->translation.y;
        result.At(2, 3) = this->translation.z;
        return result;
    }

    template <typename T>
    Vec<T, 3> Transform<T>::TransformPosition(const Vec<T, 3>& inPosition) const
    {
        return this->rotation.RotateVector(this->scale * inPosition) + this->translation;
    }

    template <typename T>
    Vec<T, 4> Transform<T>::TransformPosition(const Vec<T, 4>& inPosition) const
    {
        const Vec<T, 3> scaled(inPosition.x * this->scale.x, inPosition.y * this->scale.y, inPosition.z * this->scale.z);
        const Vec<T, 3> transformed = this->rotation.RotateVector(scaled) + this->translation * inPosition.w;
        return Vec<T, 4>(transformed.x, transformed.y, transformed.z, inPosition.w);
    }

    template <typename T>
    template <typename IT>
    Transform<IT> Transform<T>::CastTo() const
    {
        Transform<IT> result;
        result.translation = this->translation.template CastTo<IT>();
        result.rotation = this->rotation.template CastTo<IT>();
        result.scale = this->scale.template CastTo<IT>();
        return result;
    }

    template <typename T>
    requires FloatingPoint<T>
    bool AlmostEqual(const Transform<T>& lhs, const Transform<T>& rhs, T absoluteTolerance, T relativeTolerance)
    {
        return AlmostEqual(lhs.scale, rhs.scale, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.rotation, rhs.rotation, absoluteTolerance, relativeTolerance)
            && AlmostEqual(lhs.translation, rhs.translation, absoluteTolerance, relativeTolerance);
    }
}

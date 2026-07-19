//
// Created by johnk on 2023/5/9.
//

#pragma once

#include <bit>
#include <cstdint>

#include <Common/Math/Common.h>

namespace Common {
    template <std::endian E> concept ValidEndian = E == std::endian::little || E == std::endian::big;

    template <std::endian E>
    requires ValidEndian<E>
    struct HalfFloatBase {
        explicit HalfFloatBase(uint16_t inValue);

        uint16_t value;
    };

    template <std::endian E>
    struct HalfFloat : HalfFloatBase<E> {
        HalfFloat();
        HalfFloat(const HalfFloat& inValue);
        HalfFloat(HalfFloat&& inValue) noexcept;
        HalfFloat(float inValue); // NOLINT

        HalfFloat& operator=(float inValue);
        HalfFloat& operator=(const HalfFloat& inValue);

        void Set(float inValue);
        float AsFloat() const;
        operator float() const; // NOLINT

        bool operator==(float rhs) const;
        bool operator!=(float rhs) const;
        bool operator==(const HalfFloat& rhs) const;
        bool operator!=(const HalfFloat& rhs) const;
        bool operator>(const HalfFloat& rhs) const;
        bool operator<(const HalfFloat& rhs) const;
        bool operator>=(const HalfFloat& rhs) const;
        bool operator<=(const HalfFloat& rhs) const;
        HalfFloat operator+(float rhs) const;
        HalfFloat operator-(float rhs) const;
        HalfFloat operator*(float rhs) const;
        HalfFloat operator/(float rhs) const;
        HalfFloat operator+(const HalfFloat& rhs) const;
        HalfFloat operator-(const HalfFloat& rhs) const;
        HalfFloat operator*(const HalfFloat& rhs) const;
        HalfFloat operator/(const HalfFloat& rhs) const;
        HalfFloat& operator+=(float rhs);
        HalfFloat& operator-=(float rhs);
        HalfFloat& operator*=(float rhs);
        HalfFloat& operator/=(float rhs);
        HalfFloat& operator+=(const HalfFloat& rhs);
        HalfFloat& operator-=(const HalfFloat& rhs);
        HalfFloat& operator*=(const HalfFloat& rhs);
        HalfFloat& operator/=(const HalfFloat& rhs);
    };

    using HFloat = HalfFloat<std::endian::native>;

    template <typename T> concept HalfFloatingPoint = std::is_same_v<T, HFloat>;
    template <typename T> concept FloatingPoint = std::is_floating_point_v<T> || HalfFloatingPoint<T>;

    template <std::endian E>
    bool AlmostEqual(HalfFloat<E> lhs, HalfFloat<E> rhs, HalfFloat<E> absoluteTolerance = DefaultTolerance<HalfFloat<E>>(), HalfFloat<E> relativeTolerance = DefaultTolerance<HalfFloat<E>>());
}

namespace Common::Internal {
    inline uint16_t FloatToHalfBits(float inValue)
    {
        const uint32_t floatBits = std::bit_cast<uint32_t>(inValue);
        const uint16_t sign = static_cast<uint16_t>((floatBits >> 16) & 0x8000u);
        const uint32_t exponent = (floatBits >> 23) & 0xffu;
        const uint32_t mantissa = floatBits & 0x7fffffu;

        if (exponent == 0xffu) {
            if (mantissa == 0) {
                return static_cast<uint16_t>(sign | 0x7c00u);
            }

            uint16_t payload = static_cast<uint16_t>(mantissa >> 13);
            if (payload == 0) {
                payload = 1;
            }
            return static_cast<uint16_t>(sign | 0x7c00u | payload);
        }

        const int32_t halfExponent = static_cast<int32_t>(exponent) - 127 + 15;
        if (halfExponent >= 31) {
            return static_cast<uint16_t>(sign | 0x7c00u);
        }

        if (halfExponent <= 0) {
            if (halfExponent < -10) {
                return sign;
            }

            const uint32_t significand = mantissa | 0x800000u;
            const uint32_t shift = static_cast<uint32_t>(14 - halfExponent);
            uint16_t halfMantissa = static_cast<uint16_t>(significand >> shift);
            const uint32_t remainderMask = (uint32_t(1) << shift) - 1;
            const uint32_t remainder = significand & remainderMask;
            const uint32_t halfway = uint32_t(1) << (shift - 1);
            if (remainder > halfway || (remainder == halfway && (halfMantissa & 1u) != 0)) {
                ++halfMantissa;
            }
            return static_cast<uint16_t>(sign | halfMantissa);
        }

        uint16_t result = static_cast<uint16_t>(sign | (static_cast<uint16_t>(halfExponent) << 10) | static_cast<uint16_t>(mantissa >> 13));
        const uint32_t remainder = mantissa & 0x1fffu;
        if (remainder > 0x1000u || (remainder == 0x1000u && (result & 1u) != 0)) {
            ++result;
        }
        return result;
    }

    inline float HalfBitsToFloat(uint16_t inValue)
    {
        const uint32_t sign = static_cast<uint32_t>(inValue & 0x8000u) << 16;
        const uint32_t exponent = (inValue >> 10) & 0x1fu;
        uint32_t mantissa = inValue & 0x03ffu;
        uint32_t floatBits;

        if (exponent == 0) {
            if (mantissa == 0) {
                floatBits = sign;
            } else {
                const uint32_t shift = std::countl_zero(mantissa) - 21;
                mantissa = (mantissa << shift) & 0x03ffu;
                const uint32_t floatExponent = 127 - 14 - shift;
                floatBits = sign | (floatExponent << 23) | (mantissa << 13);
            }
        } else if (exponent == 0x1fu) {
            floatBits = sign | 0x7f800000u | (mantissa << 13);
        } else {
            const uint32_t floatExponent = exponent - 15 + 127;
            floatBits = sign | (floatExponent << 23) | (mantissa << 13);
        }

        return std::bit_cast<float>(floatBits);
    }
}

namespace Common {
    template <std::endian E>
    bool AlmostEqual(HalfFloat<E> lhs, HalfFloat<E> rhs, HalfFloat<E> absoluteTolerance, HalfFloat<E> relativeTolerance)
    {
        return AlmostEqual(lhs.AsFloat(), rhs.AsFloat(), absoluteTolerance.AsFloat(), relativeTolerance.AsFloat());
    }

    template <std::endian E>
    requires ValidEndian<E>
    HalfFloatBase<E>::HalfFloatBase(uint16_t inValue)
        : value(inValue)
    {
    }

    template <std::endian E>
    HalfFloat<E>::HalfFloat()
        : HalfFloatBase<E>(0)
    {
    }

    template <std::endian E>
    HalfFloat<E>::HalfFloat(const HalfFloat& inValue) = default;

    template <std::endian E>
    HalfFloat<E>::HalfFloat(HalfFloat&& inValue) noexcept = default;

    template <std::endian E>
    HalfFloat<E>::HalfFloat(float inValue)
        : HalfFloatBase<E>(Internal::FloatToHalfBits(inValue))
    {
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator=(float inValue)
    {
        Set(inValue);
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator=(const HalfFloat& inValue) = default;

    template <std::endian E>
    void HalfFloat<E>::Set(float inValue)
    {
        this->value = Internal::FloatToHalfBits(inValue);
    }

    template <std::endian E>
    float HalfFloat<E>::AsFloat() const
    {
        return Internal::HalfBitsToFloat(this->value);
    }

    template <std::endian E>
    HalfFloat<E>::operator float() const
    {
        return AsFloat();
    }

    template <std::endian E>
    bool HalfFloat<E>::operator==(float rhs) const
    {
        return AsFloat() == rhs;
    }

    template <std::endian E>
    bool HalfFloat<E>::operator!=(float rhs) const
    {
        return !this->operator==(rhs);
    }

    template <std::endian E>
    bool HalfFloat<E>::operator==(const HalfFloat& rhs) const
    {
        return AsFloat() == rhs.AsFloat();
    }

    template <std::endian E>
    bool HalfFloat<E>::operator!=(const HalfFloat& rhs) const
    {
        return !this->operator==(rhs);
    }

    template <std::endian E>
    bool HalfFloat<E>::operator>(const HalfFloat& rhs) const
    {
        return AsFloat() > rhs.AsFloat();
    }

    template <std::endian E>
    bool HalfFloat<E>::operator<(const HalfFloat& rhs) const
    {
        return AsFloat() < rhs.AsFloat();
    }

    template <std::endian E>
    bool HalfFloat<E>::operator>=(const HalfFloat& rhs) const
    {
        return AsFloat() >= rhs.AsFloat();
    }

    template <std::endian E>
    bool HalfFloat<E>::operator<=(const HalfFloat& rhs) const
    {
        return AsFloat() <= rhs.AsFloat();
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator+(float rhs) const
    {
        return { AsFloat() + rhs };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator-(float rhs) const
    {
        return { AsFloat() - rhs };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator*(float rhs) const
    {
        return { AsFloat() * rhs };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator/(float rhs) const
    {
        return { AsFloat() / rhs };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator+(const HalfFloat& rhs) const
    {
        return { AsFloat() + rhs.AsFloat() };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator-(const HalfFloat& rhs) const
    {
        return { AsFloat() - rhs.AsFloat() };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator*(const HalfFloat& rhs) const
    {
        return { AsFloat() * rhs.AsFloat() };
    }

    template <std::endian E>
    HalfFloat<E> HalfFloat<E>::operator/(const HalfFloat& rhs) const
    {
        return { AsFloat() / rhs.AsFloat() };
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator+=(float rhs)
    {
        Set(AsFloat() + rhs);
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator-=(float rhs)
    {
        Set(AsFloat() - rhs);
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator*=(float rhs)
    {
        Set(AsFloat() * rhs);
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator/=(float rhs)
    {
        Set(AsFloat() / rhs);
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator+=(const HalfFloat& rhs)
    {
        Set(AsFloat() + rhs.AsFloat());
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator-=(const HalfFloat& rhs)
    {
        Set(AsFloat() - rhs.AsFloat());
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator*=(const HalfFloat& rhs)
    {
        Set(AsFloat() * rhs.AsFloat());
        return *this;
    }

    template <std::endian E>
    HalfFloat<E>& HalfFloat<E>::operator/=(const HalfFloat& rhs)
    {
        Set(AsFloat() / rhs.AsFloat());
        return *this;
    }
}

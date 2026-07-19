#pragma once

#include <Common/Math/Box.h>
#include <Common/Math/Color.h>
#include <Common/Math/Half.h>
#include <Common/Math/Matrix.h>
#include <Common/Math/Projection.h>
#include <Common/Math/Quaternion.h>
#include <Common/Math/Rect.h>
#include <Common/Math/Sphere.h>
#include <Common/Math/Transform.h>
#include <Common/Math/Vector.h>
#include <Common/Math/View.h>
#include <Common/Serialization.h>
#include <Common/String.h>

namespace Common {
    template <std::endian E>
    struct Serializer<HalfFloat<E>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Internal::HalfFloat");

        static size_t Serialize(BinarySerializeStream& stream, const HalfFloat<E>& value)
        {
            return Serializer<uint16_t>::Serialize(stream, value.value);
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, HalfFloat<E>& value)
        {
            return Serializer<uint16_t>::Deserialize(stream, value.value);
        }
    };

    template <std::endian E>
    struct StringConverter<HalfFloat<E>> {
        static std::string ToString(const HalfFloat<E>& inValue)
        {
            return StringConverter<float>::ToString(inValue.AsFloat());
        }
    };

    template <std::endian E>
    struct JsonSerializer<HalfFloat<E>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const HalfFloat<E>& inValue)
        {
            JsonSerializer<float>::JsonSerialize(outJsonValue, inAllocator, inValue.AsFloat());
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, HalfFloat<E>& outValue)
        {
            float floatValue = 0.0f;
            JsonSerializer<float>::JsonDeserialize(inJsonValue, floatValue);
            outValue = floatValue;
        }
    };

    template <Serializable T, uint8_t L, MathBackend B>
    struct Serializer<Vec<T, L, B>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Vector") + Serializer<T>::typeId + L;

        static size_t Serialize(BinarySerializeStream& stream, const Vec<T, L, B>& value)
        {
            size_t serialized = 0;
            for (auto i = 0; i < L; i++) {
                serialized += Serializer<T>::Serialize(stream, value.data[i]);
            }
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Vec<T, L, B>& value)
        {
            size_t deserialized = 0;
            for (auto i = 0; i < L; i++) {
                deserialized += Serializer<T>::Deserialize(stream, value.data[i]);
            }
            return deserialized;
        }
    };

    template <StringConvertible T, uint8_t L, MathBackend B>
    struct StringConverter<Vec<T, L, B>> {
        static std::string ToString(const Vec<T, L, B>& inValue)
        {
            std::stringstream stream;
            stream << "(";
            for (auto i = 0; i < L; i++) {
                stream << StringConverter<T>::ToString(inValue.data[i]);
                if (i != L - 1) {
                    stream << ", ";
                }
            }
            stream << ")";
            return stream.str();
        }
    };

    template <JsonSerializable T, uint8_t L, MathBackend B>
    struct JsonSerializer<Vec<T, L, B>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Vec<T, L, B>& inValue)
        {
            outJsonValue.SetArray();
            outJsonValue.Reserve(L, inAllocator);
            for (auto i = 0; i < L; i++) {
                rapidjson::Value elementJson;
                JsonSerializer<T>::JsonSerialize(elementJson, inAllocator, inValue[i]);
                outJsonValue.PushBack(elementJson, inAllocator);
            }
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Vec<T, L, B>& outValue)
        {
            if (!inJsonValue.IsArray() || inJsonValue.Size() != L) {
                return;
            }
            for (auto i = 0; i < L; i++) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue[i], outValue[i]);
            }
        }
    };

    template <Serializable T, uint8_t R, uint8_t C, MathBackend B>
    struct Serializer<Mat<T, R, C, B>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Matrix") + Serializer<T>::typeId + (R << 8) + C;

        static size_t Serialize(BinarySerializeStream& stream, const Mat<T, R, C, B>& value)
        {
            size_t serialized = 0;
            for (auto i = 0; i < R * C; i++) {
                serialized += Serializer<T>::Serialize(stream, value.data[i]);
            }
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Mat<T, R, C, B>& value)
        {
            size_t deserialized = 0;
            for (auto i = 0; i < R * C; i++) {
                deserialized += Serializer<T>::Deserialize(stream, value.data[i]);
            }
            return deserialized;
        }
    };

    template <StringConvertible T, uint8_t R, uint8_t C, MathBackend B>
    struct StringConverter<Mat<T, R, C, B>> {
        static std::string ToString(const Mat<T, R, C, B>& inValue)
        {
            std::stringstream stream;
            stream << "(";
            for (auto i = 0; i < R; i++) {
                for (auto j = 0; j < C; j++) {
                    stream << StringConverter<T>::ToString(inValue.At(i, j));
                    if (i * C + j != R * C - 1) {
                        stream << ", ";
                    }
                }
            }
            stream << ")";
            return stream.str();
        }
    };

    template <JsonSerializable T, uint8_t R, uint8_t C, MathBackend B>
    struct JsonSerializer<Mat<T, R, C, B>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Mat<T, R, C, B>& inValue)
        {
            outJsonValue.SetArray();
            outJsonValue.Reserve(R * C, inAllocator);
            for (auto i = 0; i < R; i++) {
                for (auto j = 0; j < C; j++) {
                    rapidjson::Value jsonElement;
                    JsonSerializer<T>::JsonSerialize(jsonElement, inAllocator, inValue.At(i, j));
                    outJsonValue.PushBack(jsonElement, inAllocator);
                }
            }
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Mat<T, R, C, B>& outValue)
        {
            if (!inJsonValue.IsArray() || inJsonValue.Size() != R * C) {
                return;
            }
            for (auto i = 0; i < inJsonValue.Size(); i++) {
                T element;
                JsonSerializer<T>::JsonDeserialize(inJsonValue[i], element);
                outValue.At(i / C, i % C) = std::move(element);
            }
        }
    };

    template <Serializable T>
    struct Serializer<Angle<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Angle") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Angle<T>& value)
        {
            return Serializer<T>::Serialize(stream, value.value);
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Angle<T>& value)
        {
            return Serializer<T>::Deserialize(stream, value.value);
        }
    };

    template <Serializable T>
    struct Serializer<Radian<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Radian") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Radian<T>& value)
        {
            return Serializer<T>::Serialize(stream, value.value);
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Radian<T>& value)
        {
            return Serializer<T>::Deserialize(stream, value.value);
        }
    };

    template <Serializable T, MathBackend B>
    struct Serializer<Quaternion<T, B>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Quaternion") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Quaternion<T, B>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<T>::Serialize(stream, value.w);
            serialized += Serializer<T>::Serialize(stream, value.x);
            serialized += Serializer<T>::Serialize(stream, value.y);
            serialized += Serializer<T>::Serialize(stream, value.z);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Quaternion<T, B>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<T>::Deserialize(stream, value.w);
            deserialized += Serializer<T>::Deserialize(stream, value.x);
            deserialized += Serializer<T>::Deserialize(stream, value.y);
            deserialized += Serializer<T>::Deserialize(stream, value.z);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<Angle<T>> {
        static std::string ToString(const Angle<T>& inValue)
        {
            return std::format("a{}", StringConverter<T>::ToString(inValue.value));
        }
    };

    template <StringConvertible T>
    struct StringConverter<Radian<T>> {
        static std::string ToString(const Radian<T>& inValue)
        {
            return StringConverter<T>::ToString(inValue.value);
        }
    };

    template <StringConvertible T, MathBackend B>
    struct StringConverter<Quaternion<T, B>> {
        static std::string ToString(const Quaternion<T, B>& inValue)
        {
            return std::format(
                "({}, {}, {}, {})",
                StringConverter<T>::ToString(inValue.w),
                StringConverter<T>::ToString(inValue.x),
                StringConverter<T>::ToString(inValue.y),
                StringConverter<T>::ToString(inValue.z));
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Angle<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Angle<T>& inValue)
        {
            JsonSerializer<T>::JsonSerialize(outJsonValue, inAllocator, inValue.value);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Angle<T>& outValue)
        {
            JsonSerializer<T>::JsonDeserialize(inJsonValue, outValue.value);
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Radian<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Radian<T>& inValue)
        {
            JsonSerializer<T>::JsonSerialize(outJsonValue, inAllocator, inValue.value);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Radian<T>& outValue)
        {
            JsonSerializer<T>::JsonDeserialize(inJsonValue, outValue.value);
        }
    };

    template <JsonSerializable T, MathBackend B>
    struct JsonSerializer<Quaternion<T, B>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Quaternion<T, B>& inValue)
        {
            outJsonValue.SetArray();
            rapidjson::Value wJson;
            rapidjson::Value xJson;
            rapidjson::Value yJson;
            rapidjson::Value zJson;
            JsonSerializer<T>::JsonSerialize(wJson, inAllocator, inValue.w);
            JsonSerializer<T>::JsonSerialize(xJson, inAllocator, inValue.x);
            JsonSerializer<T>::JsonSerialize(yJson, inAllocator, inValue.y);
            JsonSerializer<T>::JsonSerialize(zJson, inAllocator, inValue.z);
            outJsonValue.PushBack(wJson, inAllocator);
            outJsonValue.PushBack(xJson, inAllocator);
            outJsonValue.PushBack(yJson, inAllocator);
            outJsonValue.PushBack(zJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Quaternion<T, B>& outValue)
        {
            if (!inJsonValue.IsArray() || inJsonValue.Size() != 4) {
                return;
            }
            JsonSerializer<T>::JsonDeserialize(inJsonValue[0], outValue.w);
            JsonSerializer<T>::JsonDeserialize(inJsonValue[1], outValue.x);
            JsonSerializer<T>::JsonDeserialize(inJsonValue[2], outValue.y);
            JsonSerializer<T>::JsonDeserialize(inJsonValue[3], outValue.z);
        }
    };

    template <Serializable T>
    struct Serializer<Rect<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Rect") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Rect<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<Vec<T, 2>>::Serialize(stream, value.min);
            serialized += Serializer<Vec<T, 2>>::Serialize(stream, value.max);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Rect<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<Vec<T, 2>>::Deserialize(stream, value.min);
            deserialized += Serializer<Vec<T, 2>>::Deserialize(stream, value.max);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<Rect<T>> {
        static std::string ToString(const Rect<T>& inValue)
        {
            return std::format(
                "{}min={}, max={}{}", "{", StringConverter<Vec<T, 2>>::ToString(inValue.min),
                StringConverter<Vec<T, 2>>::ToString(inValue.max), "}");
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Rect<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Rect<T>& inValue)
        {
            rapidjson::Value minJson;
            rapidjson::Value maxJson;
            JsonSerializer<Vec<T, 2>>::JsonSerialize(minJson, inAllocator, inValue.min);
            JsonSerializer<Vec<T, 2>>::JsonSerialize(maxJson, inAllocator, inValue.max);
            outJsonValue.SetObject();
            outJsonValue.AddMember("min", minJson, inAllocator);
            outJsonValue.AddMember("max", maxJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Rect<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("min")) {
                JsonSerializer<Vec<T, 2>>::JsonDeserialize(inJsonValue["min"], outValue.min);
            }
            if (inJsonValue.HasMember("max")) {
                JsonSerializer<Vec<T, 2>>::JsonDeserialize(inJsonValue["max"], outValue.max);
            }
        }
    };

    template <Serializable T>
    struct Serializer<Box<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Box") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Box<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<Vec<T, 3>>::Serialize(stream, value.min);
            serialized += Serializer<Vec<T, 3>>::Serialize(stream, value.max);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Box<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<Vec<T, 3>>::Deserialize(stream, value.min);
            deserialized += Serializer<Vec<T, 3>>::Deserialize(stream, value.max);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<Box<T>> {
        static std::string ToString(const Box<T>& inValue)
        {
            return std::format(
                "{}min={}, max={}{}", "{", StringConverter<Vec<T, 3>>::ToString(inValue.min),
                StringConverter<Vec<T, 3>>::ToString(inValue.max), "}");
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Box<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Box<T>& inValue)
        {
            rapidjson::Value minJson;
            rapidjson::Value maxJson;
            JsonSerializer<Vec<T, 3>>::JsonSerialize(minJson, inAllocator, inValue.min);
            JsonSerializer<Vec<T, 3>>::JsonSerialize(maxJson, inAllocator, inValue.max);
            outJsonValue.SetObject();
            outJsonValue.AddMember("min", minJson, inAllocator);
            outJsonValue.AddMember("max", maxJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Box<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("min")) {
                JsonSerializer<Vec<T, 3>>::JsonDeserialize(inJsonValue["min"], outValue.min);
            }
            if (inJsonValue.HasMember("max")) {
                JsonSerializer<Vec<T, 3>>::JsonDeserialize(inJsonValue["max"], outValue.max);
            }
        }
    };

    template <Serializable T>
    struct Serializer<Sphere<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Sphere") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Sphere<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<Vec<T, 3>>::Serialize(stream, value.center);
            serialized += Serializer<T>::Serialize(stream, value.radius);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Sphere<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<Vec<T, 3>>::Deserialize(stream, value.center);
            deserialized += Serializer<T>::Deserialize(stream, value.radius);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<Sphere<T>> {
        static std::string ToString(const Sphere<T>& inValue)
        {
            return std::format(
                "{}center={}, radius={}{}", "{", StringConverter<Vec<T, 3>>::ToString(inValue.center),
                StringConverter<T>::ToString(inValue.radius), "}");
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Sphere<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Sphere<T>& inValue)
        {
            rapidjson::Value centerJson;
            rapidjson::Value radiusJson;
            JsonSerializer<Vec<T, 3>>::JsonSerialize(centerJson, inAllocator, inValue.center);
            JsonSerializer<T>::JsonSerialize(radiusJson, inAllocator, inValue.radius);
            outJsonValue.SetObject();
            outJsonValue.AddMember("center", centerJson, inAllocator);
            outJsonValue.AddMember("radius", radiusJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Sphere<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("center")) {
                JsonSerializer<Vec<T, 3>>::JsonDeserialize(inJsonValue["center"], outValue.center);
            }
            if (inJsonValue.HasMember("radius")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["radius"], outValue.radius);
            }
        }
    };

    template <>
    struct Serializer<Color> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Color");

        static size_t Serialize(BinarySerializeStream& stream, const Color& value)
        {
            size_t serialized = 0;
            serialized += Serializer<uint8_t>::Serialize(stream, value.r);
            serialized += Serializer<uint8_t>::Serialize(stream, value.g);
            serialized += Serializer<uint8_t>::Serialize(stream, value.b);
            serialized += Serializer<uint8_t>::Serialize(stream, value.a);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Color& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<uint8_t>::Deserialize(stream, value.r);
            deserialized += Serializer<uint8_t>::Deserialize(stream, value.g);
            deserialized += Serializer<uint8_t>::Deserialize(stream, value.b);
            deserialized += Serializer<uint8_t>::Deserialize(stream, value.a);
            return deserialized;
        }
    };

    template <>
    struct Serializer<LinearColor> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::LinearColor");

        static size_t Serialize(BinarySerializeStream& stream, const LinearColor& value)
        {
            size_t serialized = 0;
            serialized += Serializer<float>::Serialize(stream, value.r);
            serialized += Serializer<float>::Serialize(stream, value.g);
            serialized += Serializer<float>::Serialize(stream, value.b);
            serialized += Serializer<float>::Serialize(stream, value.a);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, LinearColor& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<float>::Deserialize(stream, value.r);
            deserialized += Serializer<float>::Deserialize(stream, value.g);
            deserialized += Serializer<float>::Deserialize(stream, value.b);
            deserialized += Serializer<float>::Deserialize(stream, value.a);
            return deserialized;
        }
    };

    template <>
    struct StringConverter<Color> {
        static std::string ToString(const Color& inValue)
        {
            return std::format(
                "{}r={}, g={}, b={}, a={}{}", "{", StringConverter<uint8_t>::ToString(inValue.r),
                StringConverter<uint8_t>::ToString(inValue.g), StringConverter<uint8_t>::ToString(inValue.b),
                StringConverter<uint8_t>::ToString(inValue.a), "}");
        }
    };

    template <>
    struct StringConverter<LinearColor> {
        static std::string ToString(const LinearColor& inValue)
        {
            return std::format(
                "{}r={}, g={}, b={}, a={}{}", "{", StringConverter<float>::ToString(inValue.r),
                StringConverter<float>::ToString(inValue.g), StringConverter<float>::ToString(inValue.b),
                StringConverter<float>::ToString(inValue.a), "}");
        }
    };

    template <>
    struct JsonSerializer<Color> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Color& inValue)
        {
            outJsonValue.SetObject();
            outJsonValue.AddMember("r", inValue.r, inAllocator);
            outJsonValue.AddMember("g", inValue.g, inAllocator);
            outJsonValue.AddMember("b", inValue.b, inAllocator);
            outJsonValue.AddMember("a", inValue.a, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Color& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("r") && inJsonValue["r"].IsUint()) {
                outValue.r = static_cast<uint8_t>(inJsonValue["r"].GetUint());
            }
            if (inJsonValue.HasMember("g") && inJsonValue["g"].IsUint()) {
                outValue.g = static_cast<uint8_t>(inJsonValue["g"].GetUint());
            }
            if (inJsonValue.HasMember("b") && inJsonValue["b"].IsUint()) {
                outValue.b = static_cast<uint8_t>(inJsonValue["b"].GetUint());
            }
            if (inJsonValue.HasMember("a") && inJsonValue["a"].IsUint()) {
                outValue.a = static_cast<uint8_t>(inJsonValue["a"].GetUint());
            }
        }
    };

    template <>
    struct JsonSerializer<LinearColor> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const LinearColor& inValue)
        {
            outJsonValue.SetObject();
            outJsonValue.AddMember("r", inValue.r, inAllocator);
            outJsonValue.AddMember("g", inValue.g, inAllocator);
            outJsonValue.AddMember("b", inValue.b, inAllocator);
            outJsonValue.AddMember("a", inValue.a, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, LinearColor& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("r") && inJsonValue["r"].IsFloat()) {
                outValue.r = inJsonValue["r"].GetFloat();
            }
            if (inJsonValue.HasMember("g") && inJsonValue["g"].IsFloat()) {
                outValue.g = inJsonValue["g"].GetFloat();
            }
            if (inJsonValue.HasMember("b") && inJsonValue["b"].IsFloat()) {
                outValue.b = inJsonValue["b"].GetFloat();
            }
            if (inJsonValue.HasMember("a") && inJsonValue["a"].IsFloat()) {
                outValue.a = inJsonValue["a"].GetFloat();
            }
        }
    };

    template <Serializable T>
    struct Serializer<Transform<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::Transform") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const Transform<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<Vec<T, 3>>::Serialize(stream, value.scale);
            serialized += Serializer<Quaternion<T>>::Serialize(stream, value.rotation);
            serialized += Serializer<Vec<T, 3>>::Serialize(stream, value.translation);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, Transform<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<Vec<T, 3>>::Deserialize(stream, value.scale);
            deserialized += Serializer<Quaternion<T>>::Deserialize(stream, value.rotation);
            deserialized += Serializer<Vec<T, 3>>::Deserialize(stream, value.translation);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<Transform<T>> {
        static std::string ToString(const Transform<T>& inValue)
        {
            return std::format(
                "{}scale={}, rotation={}, translation={}{}", "{",
                StringConverter<Vec<T, 3>>::ToString(inValue.scale),
                StringConverter<Quaternion<T>>::ToString(inValue.rotation),
                StringConverter<Vec<T, 3>>::ToString(inValue.translation), "}");
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<Transform<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const Transform<T>& inValue)
        {
            rapidjson::Value scaleJson;
            rapidjson::Value rotationJson;
            rapidjson::Value translationJson;
            JsonSerializer<Vec<T, 3>>::JsonSerialize(scaleJson, inAllocator, inValue.scale);
            JsonSerializer<Quaternion<T>>::JsonSerialize(rotationJson, inAllocator, inValue.rotation);
            JsonSerializer<Vec<T, 3>>::JsonSerialize(translationJson, inAllocator, inValue.translation);
            outJsonValue.SetObject();
            outJsonValue.AddMember("scale", scaleJson, inAllocator);
            outJsonValue.AddMember("rotation", rotationJson, inAllocator);
            outJsonValue.AddMember("translation", translationJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, Transform<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("scale")) {
                JsonSerializer<Vec<T, 3>>::JsonDeserialize(inJsonValue["scale"], outValue.scale);
            }
            if (inJsonValue.HasMember("rotation")) {
                JsonSerializer<Quaternion<T>>::JsonDeserialize(inJsonValue["rotation"], outValue.rotation);
            }
            if (inJsonValue.HasMember("translation")) {
                JsonSerializer<Vec<T, 3>>::JsonDeserialize(inJsonValue["translation"], outValue.translation);
            }
        }
    };

    template <Serializable T>
    struct Serializer<ViewTransform<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::ViewTransform") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const ViewTransform<T>& value)
        {
            return Serializer<Transform<T>>::Serialize(stream, value);
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, ViewTransform<T>& value)
        {
            return Serializer<Transform<T>>::Deserialize(stream, value);
        }
    };

    template <StringConvertible T>
    struct StringConverter<ViewTransform<T>> {
        static std::string ToString(const ViewTransform<T>& inValue)
        {
            return StringConverter<Transform<T>>::ToString(inValue);
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<ViewTransform<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const ViewTransform<T>& inValue)
        {
            JsonSerializer<Transform<T>>::JsonSerialize(outJsonValue, inAllocator, inValue);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, ViewTransform<T>& outValue)
        {
            JsonSerializer<Transform<T>>::JsonDeserialize(inJsonValue, outValue);
        }
    };

    template <Serializable T>
    struct Serializer<ReversedZOrthogonalProjection<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::ReversedZOrthogonalProjection") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const ReversedZOrthogonalProjection<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<T>::Serialize(stream, value.width);
            serialized += Serializer<T>::Serialize(stream, value.height);
            serialized += Serializer<T>::Serialize(stream, value.nearPlane);
            serialized += Serializer<std::optional<T>>::Serialize(stream, value.farPlane);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, ReversedZOrthogonalProjection<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<T>::Deserialize(stream, value.width);
            deserialized += Serializer<T>::Deserialize(stream, value.height);
            deserialized += Serializer<T>::Deserialize(stream, value.nearPlane);
            deserialized += Serializer<std::optional<T>>::Deserialize(stream, value.farPlane);
            return deserialized;
        }
    };

    template <Serializable T>
    struct Serializer<ReversedZPerspectiveProjection<T>> {
        static constexpr size_t typeId = HashUtils::StrCrc32("Common::ReversedZPerspectiveProjection") + Serializer<T>::typeId;

        static size_t Serialize(BinarySerializeStream& stream, const ReversedZPerspectiveProjection<T>& value)
        {
            size_t serialized = 0;
            serialized += Serializer<T>::Serialize(stream, value.fov);
            serialized += Serializer<T>::Serialize(stream, value.width);
            serialized += Serializer<T>::Serialize(stream, value.height);
            serialized += Serializer<T>::Serialize(stream, value.nearPlane);
            serialized += Serializer<std::optional<T>>::Serialize(stream, value.farPlane);
            return serialized;
        }

        static size_t Deserialize(BinaryDeserializeStream& stream, ReversedZPerspectiveProjection<T>& value)
        {
            size_t deserialized = 0;
            deserialized += Serializer<T>::Deserialize(stream, value.fov);
            deserialized += Serializer<T>::Deserialize(stream, value.width);
            deserialized += Serializer<T>::Deserialize(stream, value.height);
            deserialized += Serializer<T>::Deserialize(stream, value.nearPlane);
            deserialized += Serializer<std::optional<T>>::Deserialize(stream, value.farPlane);
            return deserialized;
        }
    };

    template <StringConvertible T>
    struct StringConverter<ReversedZOrthogonalProjection<T>> {
        static std::string ToString(const ReversedZOrthogonalProjection<T>& inValue)
        {
            return std::format(
                "{}width={}, height={}, near={}, far={}{}", "{", StringConverter<T>::ToString(inValue.width),
                StringConverter<T>::ToString(inValue.height), StringConverter<T>::ToString(inValue.nearPlane),
                StringConverter<std::optional<T>>::ToString(inValue.farPlane), "}");
        }
    };

    template <StringConvertible T>
    struct StringConverter<ReversedZPerspectiveProjection<T>> {
        static std::string ToString(const ReversedZPerspectiveProjection<T>& inValue)
        {
            return std::format(
                "{}fov={}, width={}, height={}, near={}, far={}{}", "{", StringConverter<T>::ToString(inValue.fov),
                StringConverter<T>::ToString(inValue.width), StringConverter<T>::ToString(inValue.height),
                StringConverter<T>::ToString(inValue.nearPlane),
                StringConverter<std::optional<T>>::ToString(inValue.farPlane), "}");
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<ReversedZOrthogonalProjection<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const ReversedZOrthogonalProjection<T>& inValue)
        {
            rapidjson::Value widthJson;
            rapidjson::Value heightJson;
            rapidjson::Value nearJson;
            rapidjson::Value farJson;
            JsonSerializer<T>::JsonSerialize(widthJson, inAllocator, inValue.width);
            JsonSerializer<T>::JsonSerialize(heightJson, inAllocator, inValue.height);
            JsonSerializer<T>::JsonSerialize(nearJson, inAllocator, inValue.nearPlane);
            JsonSerializer<std::optional<T>>::JsonSerialize(farJson, inAllocator, inValue.farPlane);
            outJsonValue.SetObject();
            outJsonValue.AddMember("width", widthJson, inAllocator);
            outJsonValue.AddMember("height", heightJson, inAllocator);
            outJsonValue.AddMember("near", nearJson, inAllocator);
            outJsonValue.AddMember("far", farJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, ReversedZOrthogonalProjection<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("width")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["width"], outValue.width);
            }
            if (inJsonValue.HasMember("height")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["height"], outValue.height);
            }
            if (inJsonValue.HasMember("near")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["near"], outValue.nearPlane);
            }
            if (inJsonValue.HasMember("far")) {
                JsonSerializer<std::optional<T>>::JsonDeserialize(inJsonValue["far"], outValue.farPlane);
            }
        }
    };

    template <JsonSerializable T>
    struct JsonSerializer<ReversedZPerspectiveProjection<T>> {
        static void JsonSerialize(rapidjson::Value& outJsonValue, rapidjson::Document::AllocatorType& inAllocator, const ReversedZPerspectiveProjection<T>& inValue)
        {
            rapidjson::Value fovJson;
            rapidjson::Value widthJson;
            rapidjson::Value heightJson;
            rapidjson::Value nearJson;
            rapidjson::Value farJson;
            JsonSerializer<T>::JsonSerialize(fovJson, inAllocator, inValue.fov);
            JsonSerializer<T>::JsonSerialize(widthJson, inAllocator, inValue.width);
            JsonSerializer<T>::JsonSerialize(heightJson, inAllocator, inValue.height);
            JsonSerializer<T>::JsonSerialize(nearJson, inAllocator, inValue.nearPlane);
            JsonSerializer<std::optional<T>>::JsonSerialize(farJson, inAllocator, inValue.farPlane);
            outJsonValue.SetObject();
            outJsonValue.AddMember("fov", fovJson, inAllocator);
            outJsonValue.AddMember("width", widthJson, inAllocator);
            outJsonValue.AddMember("height", heightJson, inAllocator);
            outJsonValue.AddMember("near", nearJson, inAllocator);
            outJsonValue.AddMember("far", farJson, inAllocator);
        }

        static void JsonDeserialize(const rapidjson::Value& inJsonValue, ReversedZPerspectiveProjection<T>& outValue)
        {
            if (!inJsonValue.IsObject()) {
                return;
            }
            if (inJsonValue.HasMember("fov")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["fov"], outValue.fov);
            }
            if (inJsonValue.HasMember("width")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["width"], outValue.width);
            }
            if (inJsonValue.HasMember("height")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["height"], outValue.height);
            }
            if (inJsonValue.HasMember("near")) {
                JsonSerializer<T>::JsonDeserialize(inJsonValue["near"], outValue.nearPlane);
            }
            if (inJsonValue.HasMember("far")) {
                JsonSerializer<std::optional<T>>::JsonDeserialize(inJsonValue["far"], outValue.farPlane);
            }
        }
    };
}

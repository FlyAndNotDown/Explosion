//
// Created by johnk on 2024/9/8.
//

#pragma once

#include <filesystem>

#include <rapidjson/writer.h>

#include <Test/Test.h>
#include <Common/Serialization.h>

template <typename T>
void PerformTypedSerializationTest(const T& inValue, const Test::CustomComparer<T>& inCustomCompareFunc = {})
{
    static std::filesystem::path fileName = "../Test/Generated/Common/SerializationTest.TypedSerializationTest";
    std::filesystem::create_directories(fileName.parent_path());

    {
        Common::BinaryFileSerializeStream stream(fileName.string());
        Serialize<T>(stream, inValue);
    }

    {
        T value;
        Common::BinaryFileDeserializeStream stream(fileName.string());
        Deserialize<T>(stream, value);

        if (inCustomCompareFunc) {
            ASSERT_TRUE(inCustomCompareFunc(inValue, value));
        } else {
            ASSERT_EQ(inValue, value);
        }
    }
}

template <typename T>
void PerformJsonSerializationTest(const T& inValue, const std::string& inExceptJson, const Test::CustomComparer<T>& inCustomCompareFunc = {})
{
    {
        rapidjson::Document document;

        rapidjson::Value jsonValue;
        JsonSerialize<T>(jsonValue, document.GetAllocator(), inValue);
        document.CopyFrom(jsonValue, document.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer writer(buffer);
        document.Accept(writer);

        const auto json = std::string(buffer.GetString(), buffer.GetSize());
        ASSERT_EQ(json, inExceptJson);
    }

    {
        rapidjson::Document document;
        document.Parse(inExceptJson.c_str());

        rapidjson::Value jsonValue;
        jsonValue.CopyFrom(document, document.GetAllocator());

        T value;
        JsonDeserialize<T>(jsonValue, value);
        if (inCustomCompareFunc) {
            ASSERT_TRUE(inCustomCompareFunc(inValue, value));
        } else {
            ASSERT_EQ(inValue, value);
        }
    }
}
//
// Created by johnk on 2022/7/25.
//

#include <Test/Test.h>

#include <Render/Shader.h>

class TestGlobalShaderVS final : public StaticShaderType<TestGlobalShaderVS> {
    StaticShaderInfo(
        TestGlobalShaderVS,
        "TestGlobalShader",
        RHI::ShaderStageBits::sVertex,
        "Engine/Shader/Test/TestGlobalShader.esl",
        "VSMain")

    BoolVariantField(TestBool, TEST_BOOL, false);
    RangedIntVariantField(TestRangedInt, TEST_RANGED_INT, 0, 0, 3);
    MakeVariantFieldVec(TestBool, TestRangedInt);

    BeginIncludeDirectories
        "Engine/Shader/Test"
    EndIncludeDirectories
};

ImplementStaticShaderType(TestGlobalShaderVS)

TEST(ShaderTest, StaticIncludePathAndVariantFieldsTest)
{
    const TestGlobalShaderVS shaderType;

    const std::vector<std::string> aspectIncludes = { "Engine/Shader/Test" };
    ASSERT_EQ(shaderType.GetIncludeDirectories(), aspectIncludes);

    ShaderVariantFieldVec aspectVariantFields;
    aspectVariantFields.emplace_back(ShaderBoolVariantField { "TEST_BOOL", false });
    aspectVariantFields.emplace_back(ShaderRangedIntVariantField { "TEST_RANGED_INT", 0, { 0, 3 } });
    ASSERT_EQ(shaderType.GetVariantFields(), aspectVariantFields);
}

TEST(ShaderTest, ComputeVariantKeyTest)
{
    const ShaderVariantFieldVec variantFields = {
        ShaderBoolVariantField { "TEST_BOOL", false },
        ShaderRangedIntVariantField { "TEST_RANGED_INT", 1, { 1, 4 } }
    };
    const ShaderVariantValueMap variantSet = {
        { "TEST_BOOL", true },
        { "TEST_RANGED_INT", 2 }
    };
    ASSERT_EQ(ShaderUtils::ComputeVariantKey(variantFields, variantSet), 3);
}

TEST(ShaderTest, GetAllVariantsTest)
{
    const ShaderVariantFieldVec variantFields = {
        ShaderBoolVariantField { "TEST_BOOL", false },
        ShaderRangedIntVariantField { "TEST_RANGED_INT", 1, { 1, 4 } }
    };
    const std::vector<ShaderVariantValueMap> variants = {
        { { "TEST_BOOL", false }, { "TEST_RANGED_INT", 1 } },
        { { "TEST_BOOL", true }, { "TEST_RANGED_INT", 1 } },
        { { "TEST_BOOL", false }, { "TEST_RANGED_INT", 2 } },
        { { "TEST_BOOL", true }, { "TEST_RANGED_INT", 2 } },
        { { "TEST_BOOL", false }, { "TEST_RANGED_INT", 3 } },
        { { "TEST_BOOL", true }, { "TEST_RANGED_INT", 3 } },
        { { "TEST_BOOL", false }, { "TEST_RANGED_INT", 4 } },
        { { "TEST_BOOL", true }, { "TEST_RANGED_INT", 4 } },
    };
    ASSERT_EQ(ShaderUtils::GetAllVariants(variantFields), variants);
}

TEST(ShaderTest, ComputeVariantDefinitionsTest)
{
    const ShaderVariantFieldVec variantFields = {
        ShaderBoolVariantField { "TEST_BOOL", false },
        ShaderRangedIntVariantField { "TEST_RANGED_INT", 1, { 1, 4 } }
    };
    const ShaderVariantValueMap variantSet = {
        { "TEST_BOOL", true },
        { "TEST_RANGED_INT", 2 }
    };
    const std::vector<std::string> definitions = {
        "TEST_BOOL=1",
        "TEST_RANGED_INT=2"
    };
    ASSERT_EQ(ShaderUtils::ComputeVariantDefinitions(variantFields, variantSet), definitions);
}

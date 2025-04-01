//
// Created by johnk on 2025/3/21.
//

#pragma once

#include <unordered_map>

#include <Common/Math/Vector.h>
#include <Common/Math/Matrix.h>
#include <Runtime/Asset/Asset.h>
#include <Runtime/Asset/Texture.h>
#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    enum class EEnum() MaterialType : uint8_t {
        surface,
        volume,
        postProcess,
        max
    };

    struct RUNTIME_API EClass() MaterialBoolVariantField {
        EClassBody(MaterialBoolVariantDeclare)

        MaterialBoolVariantField();

        EProperty() bool defaultValue;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialRangedUintVariantField {
        EClassBody(MaterialRangedIntVariant)

        MaterialRangedUintVariantField();

        EProperty() uint8_t defaultValue;
        EProperty() std::pair<uint8_t, uint8_t> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialBoolParameter {
        EClassBody(MaterialBoolParameter)

        MaterialBoolParameter();

        EProperty() bool defaultValue;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialIntParameter {
        EClassBody(MaterialIntParameter)

        MaterialIntParameter();

        EProperty() uint32_t defaultValue;
        EProperty() std::optional<std::pair<uint32_t, uint32_t>> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialFloatParameter {
        EClassBody(MaterialFloatParameter)

        MaterialFloatParameter();

        EProperty() float defaultValue;
        EProperty() std::optional<std::pair<float, float>> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialFVec2Parameter {
        EClassBody(MaterialVec2Parameter)

        MaterialFVec2Parameter();

        EProperty() Common::FVec2 defaultValue;
        EProperty() std::optional<std::pair<Common::FVec2, Common::FVec2>> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialFVec3Parameter {
        EClassBody(MaterialVec3Parameter)

        MaterialFVec3Parameter();

        EProperty() Common::FVec3 defaultValue;
        EProperty() std::optional<std::pair<Common::FVec3, Common::FVec3>> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialFVec4Parameter {
        EClassBody(MaterialVec4Parameter)

        MaterialFVec4Parameter();

        EProperty() Common::FVec4 defaultValue;
        EProperty() std::optional<std::pair<Common::FVec4, Common::FVec4>> range;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialFMat4x4Parameter {
        EClassBody(MaterialMat4x4Parameter)

        MaterialFMat4x4Parameter();

        EProperty() Common::FMat4x4 defaultValue;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialTextureParameter {
        EClassBody(MaterialTextureParameter)

        MaterialTextureParameter();

        EProperty() AssetPtr<Texture> defaultValue;
        EProperty() uint8_t sortPriority;
    };

    struct RUNTIME_API EClass() MaterialRenderTargetParameter {
        EClassBody(MaterialRenderTargetParameter)

        MaterialRenderTargetParameter();

        EProperty() AssetPtr<RenderTarget> defaultValue;
        EProperty() uint8_t sortPriority;
    };

    class RUNTIME_API EClass() Material final : public Asset {
        EPolyClassBody(Material)

    public:
        using VariantField = std::variant<
            MaterialBoolVariantField,
            MaterialRangedUintVariantField
        >;
        using VariantFieldMap = std::unordered_map<std::string, VariantField>;

        using Parameter = std::variant<
            MaterialBoolParameter,
            MaterialIntParameter,
            MaterialFloatParameter,
            MaterialFVec2Parameter,
            MaterialFVec3Parameter,
            MaterialFVec4Parameter,
            MaterialFMat4x4Parameter,
            MaterialTextureParameter,
            MaterialRenderTargetParameter
        >;
        using ParameterMap = std::unordered_map<std::string, Parameter>;

        explicit Material(Core::Uri inUri);

        EFunc() MaterialType GetType() const;
        EFunc() void SetType(MaterialType inType);
        EFunc() const std::string& GetSource() const;
        EFunc() void SetSource(const std::string& inSource);

        EFunc() bool HasVariantField(const std::string& inName) const;
        EFunc() VariantField& GetVariantField(const std::string& inName);
        EFunc() const VariantField& GetVariantField(const std::string& inName) const;
        EFunc() VariantField* FindVariantField(const std::string& inName);
        EFunc() const VariantField* FindVariantField(const std::string& inName) const;
        EFunc() VariantFieldMap& GetVariantFields();
        EFunc() const VariantFieldMap& GetVariantFields() const;
        EFunc() VariantField& EmplaceVariantField(const std::string& inName);

        EFunc() bool HasParameter(const std::string& inName) const;
        EFunc() Parameter& GetParameter(const std::string& inName);
        EFunc() const Parameter& GetParameter(const std::string& inName) const;
        EFunc() Parameter* FindParameter(const std::string& inName);
        EFunc() const Parameter* FindParameter(const std::string& inName) const;
        EFunc() ParameterMap& GetParameters();
        EFunc() const ParameterMap& GetParameters() const;
        EFunc() Parameter& EmplaceParameter(const std::string& inName);

    private:
        EProperty() MaterialType type;
        EProperty() std::string source;
        EProperty() VariantFieldMap variantFields;
        EProperty() ParameterMap parameters;
    };

    class RUNTIME_API EClass() MaterialInstance final : public Asset {
        EPolyClassBody(MaterialInstance)

    public:
        using VariantFieldValue = std::variant<
            bool,
            uint8_t
        >;
        using VariantFieldValueMap = std::unordered_map<std::string, VariantFieldValue>;

        using ParameterValue = std::variant<
            bool,
            int32_t,
            float,
            Common::FVec2,
            Common::FVec3,
            Common::FVec4,
            Common::FMat4x4,
            AssetPtr<Texture>,
            AssetPtr<RenderTarget>
        >;
        using ParameterValueMap = std::unordered_map<std::string, ParameterValue>;

        explicit MaterialInstance(Core::Uri inUri);

        EFunc() const AssetPtr<Material>& GetMaterial() const;
        EFunc() void SetMaterial(const AssetPtr<Material>& inMaterial);

        // TODO

    private:
        EProperty() AssetPtr<Material> material;
        EProperty() VariantFieldValueMap variantFieldValues;
        EProperty() ParameterValueMap parameterValues;
    };
}

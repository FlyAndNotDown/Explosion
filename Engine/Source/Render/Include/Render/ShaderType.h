//
// Created by johnk on 2022/7/24.
//

#pragma once

#include <cstdint>
#include <vector>
#include <utility>

#include <RHI/Common.h>

namespace Render {
    struct ShaderBoolVariantField {
        std::string macro;
        bool defaultValue;
    };

    struct ShaderRangedIntVariantField {
        std::string macro;
        int32_t defaultValue;
        std::pair<int32_t, int32_t> range;
    };

    using ShaderTypeKey = uint64_t;
    using ShaderVariantKey = uint64_t;
    using ShaderSourceHash = uint64_t;
    using ShaderVariantField = std::variant<ShaderBoolVariantField, ShaderRangedIntVariantField>;
    using ShaderVariantFieldVec = std::vector<ShaderVariantField>;
    using ShaderVariant = std::variant<bool, int32_t>;
    using ShaderVariantMap = std::unordered_map<std::string, ShaderVariant>;

    constexpr ShaderSourceHash shaderSourceHashNotCompiled = 0;

    struct VertexFactoryInput {
        std::string name;
        RHI::VertexFormat vertexFormat;
        uint32_t offset;
    };

    using VertexFactoryInputVec = std::vector<VertexFactoryInput>;

    class VertexFactoryType {
    public:
        virtual ~VertexFactoryType();

        virtual const std::string& GetName() const = 0;
        virtual const std::string& GetSourceFile() const = 0;
        virtual const VertexFactoryInputVec& GetVertexInputs() const = 0;
        virtual const ShaderVariantFieldVec& GetVariantFields() const = 0;

    protected:
        VertexFactoryType();
    };

    template <typename T>
    class StaticVertexFactoryType : public VertexFactoryType {
    public:
        StaticVertexFactoryType();
        ~StaticVertexFactoryType() override;

        const std::string& GetName() const override;
        const std::string& GetSourceFile() const override;
        const VertexFactoryInputVec& GetVertexInputs() const override;
        const ShaderVariantFieldVec& GetVariantFields() const override;
    };

    class ShaderType {
    public:
        NonCopyable(ShaderType)
        NonMovable(ShaderType)

        virtual ~ShaderType();
        virtual ShaderTypeKey GetKey() const = 0;
        virtual const std::string& GetName() const = 0;
        virtual RHI::ShaderStageBits GetStage() const = 0;
        virtual const std::string& GetSourceFile() const = 0;
        virtual const std::string& GetEntryPoint() const = 0;
        virtual const std::vector<std::string>& GetIncludeDirectories() const = 0;
        virtual const ShaderVariantFieldVec& GetVariantFields() const = 0;

    protected:
        ShaderType();
    };

    template <typename T>
    class StaticShaderType : public ShaderType {
    public:
        StaticShaderType();
        ~StaticShaderType() override;

        ShaderTypeKey GetKey() const override;
        const std::string& GetName() const override;
        RHI::ShaderStageBits GetStage() const override;
        const std::string& GetSourceFile() const override;
        const std::string& GetEntryPoint() const override;
        const std::vector<std::string>& GetIncludeDirectories() const override;
        const ShaderVariantFieldVec& GetVariantFields() const override;

    private:
        ShaderTypeKey key;
    };

    class MaterialShaderType final : public ShaderType {
    public:
        MaterialShaderType(
            const VertexFactoryType& inVertexFactory,
            std::string inName,
            RHI::ShaderStageBits inStage,
            std::string inSourceFile,
            std::string inEntryPoint,
            const std::vector<std::string>& inIncludeDirectories,
            const ShaderVariantFieldVec& inShaderVariantFields);
        ~MaterialShaderType() override;

        ShaderTypeKey GetKey() const override;
        const std::string& GetName() const override;
        RHI::ShaderStageBits GetStage() const override;
        const std::string& GetSourceFile() const override;
        const std::string& GetEntryPoint() const override;
        const std::vector<std::string>& GetIncludeDirectories() const override;
        const ShaderVariantFieldVec& GetVariantFields() const override;

    private:
        const VertexFactoryType& vertexFactory;
        std::string name;
        uint64_t key;
        RHI::ShaderStageBits stage;
        std::string sourceFile;
        std::string entryPoint;
        std::vector<std::string> includeDirectories;
        ShaderVariantFieldVec shaderVariantFields;
    };

    struct ShaderReflectionData {
        using VertexSemantic = std::string;
        using ResourceBindingName = std::string;
        using LayoutIndex = uint8_t;
        using LayoutAndResourceBinding = std::pair<LayoutIndex, RHI::ResourceBinding>;

        ShaderReflectionData();
        ShaderReflectionData(const ShaderReflectionData& inOther);
        ShaderReflectionData(ShaderReflectionData&& inOther) noexcept;
        ShaderReflectionData& operator=(const ShaderReflectionData& inOther);

        const RHI::PlatformVertexBinding& QueryVertexBindingChecked(const VertexSemantic& inSemantic) const;
        const LayoutAndResourceBinding& QueryResourceBindingChecked(const ResourceBindingName& inName) const;

        std::unordered_map<VertexSemantic, RHI::PlatformVertexBinding> vertexBindings;
        std::unordered_map<ResourceBindingName, LayoutAndResourceBinding> resourceBindings;
    };

    struct ShaderModuleData {
        std::vector<uint8_t> byteCode;
        ShaderReflectionData reflectionData;
    };

    struct ShaderInstance {
        RHI::ShaderModule* rhiHandle;
        const ShaderReflectionData* reflectionData;
    };

    class ShaderTypeRegistry {
    public:
        static ShaderTypeRegistry& Get();

        ~ShaderTypeRegistry();

        void Register(const ShaderType* inShaderType);
        void Unregister(const ShaderType* inShaderType);
        void Reset(const ShaderType* inShaderType);
        // TODO move this function to ShaderMap maybe, cause shader type registry is not good to relate with RHI::Device ?
        ShaderInstance GetShaderInstance(RHI::Device& inDevice, const ShaderType* inShaderType, const ShaderVariantMap& inShaderVariants) const;

    private:
        friend class ShaderTypeCompiler;

        struct ShaderContext {
            const ShaderType* shaderType;
            ShaderSourceHash sourceHash;
            std::unordered_map<ShaderVariantKey, ShaderModuleData> shaderModuleDatas;
            std::unordered_map<ShaderVariantKey, Common::UniquePtr<RHI::ShaderModule>> shaderModules;
        };

        static ShaderVariantKey GetShaderVariantKey(const ShaderVariantFieldVec& inFields, const ShaderVariantMap& inShaderVariants);

        ShaderTypeRegistry();

        RHI::Device& device;
        std::unordered_map<ShaderTypeKey, ShaderContext> shaderContexts;
    };
}

namespace Render {
    template <typename T>
    StaticVertexFactoryType<T>::StaticVertexFactoryType() = default;

    template <typename T>
    StaticVertexFactoryType<T>::~StaticVertexFactoryType() = default;

    template <typename T>
    const std::string& StaticVertexFactoryType<T>::GetName() const
    {
        return T::name;
    }

    template <typename T>
    const std::string& StaticVertexFactoryType<T>::GetSourceFile() const
    {
        return T::sourceFile;
    }

    template <typename T>
    const VertexFactoryInputVec& StaticVertexFactoryType<T>::GetVertexInputs() const
    {
        return T::vertexInputs;
    }

    template <typename T>
    const ShaderVariantFieldVec& StaticVertexFactoryType<T>::GetVariantFields() const
    {
        return T::variantFields;
    }

    template <typename T>
    StaticShaderType<T>::StaticShaderType()
    {
        const std::string nameStr = T::name;
        key = Common::HashUtils::CityHash(nameStr.data(), nameStr.size());
    }

    template <typename T>
    StaticShaderType<T>::~StaticShaderType() = default;

    template <typename T>
    ShaderTypeKey StaticShaderType<T>::GetKey() const
    {
        return key;
    }

    template <typename T>
    const std::string& StaticShaderType<T>::GetName() const
    {
        return T::name;
    }

    template <typename T>
    RHI::ShaderStageBits StaticShaderType<T>::GetStage() const
    {
        return T::stage;
    }

    template <typename T>
    const std::string& StaticShaderType<T>::GetSourceFile() const
    {
        return T::sourceFile;
    }

    template <typename T>
    const std::string& StaticShaderType<T>::GetEntryPoint() const
    {
        return T::entryPoint;
    }

    template <typename T>
    const std::vector<std::string>& StaticShaderType<T>::GetIncludeDirectories() const
    {
        return T::includeDirectories;
    }

    template <typename T>
    const ShaderVariantFieldVec& StaticShaderType<T>::GetVariantFields() const
    {
        return T::variantFields;
    }
}

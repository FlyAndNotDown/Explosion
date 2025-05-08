//
// Created by johnk on 2022/7/24.
//

#include <Render/ShaderType.h>

namespace Render {
    VertexFactoryType::~VertexFactoryType() = default;

    VertexFactoryType::VertexFactoryType() = default;

    ShaderType::~ShaderType()
    {
        ShaderTypeRegistry::Get().Unregister(this);
    }

    ShaderType::ShaderType()
    {
        ShaderTypeRegistry::Get().Register(this);
    }

    MaterialShaderType::MaterialShaderType(
        const VertexFactoryType& inVertexFactory,
        std::string inName,
        RHI::ShaderStageBits inStage,
        std::string inSourceFile,
        std::string inEntryPoint,
        const std::vector<std::string>& inIncludeDirectories,
        const ShaderVariantFieldVec& inShaderVariantFields)
        : vertexFactory(inVertexFactory)
        , name(std::move(inName))
        , key(Common::HashUtils::CityHash(name.data(), name.size()))
        , stage(inStage)
        , sourceFile(std::move(inSourceFile))
        , entryPoint(std::move(inEntryPoint))
        , includeDirectories(inIncludeDirectories)
        , shaderVariantFields(Common::VectorUtils::Combine(inVertexFactory.GetVariantFields(), inShaderVariantFields))
    {
    }

    MaterialShaderType::~MaterialShaderType() = default;

    ShaderTypeKey MaterialShaderType::GetKey() const
    {
        return key;
    }

    const std::string& MaterialShaderType::GetName() const
    {
        return name;
    }

    RHI::ShaderStageBits MaterialShaderType::GetStage() const
    {
        return stage;
    }

    const std::string& MaterialShaderType::GetSourceFile() const
    {
        return sourceFile;
    }

    const std::string& MaterialShaderType::GetEntryPoint() const
    {
        return entryPoint;
    }

    const std::vector<std::string>& MaterialShaderType::GetIncludeDirectories() const
    {
        return includeDirectories;
    }

    const ShaderVariantFieldVec& MaterialShaderType::GetVariantFields() const
    {
        return shaderVariantFields;
    }

    ShaderReflectionData::ShaderReflectionData() = default;

    ShaderReflectionData::ShaderReflectionData(const ShaderReflectionData& inOther) // NOLINT
        : vertexBindings(inOther.vertexBindings)
        , resourceBindings(inOther.resourceBindings)
    {
    }

    ShaderReflectionData::ShaderReflectionData(ShaderReflectionData&& inOther) noexcept // NOLINT
        : vertexBindings(std::move(inOther.vertexBindings))
        , resourceBindings(std::move(inOther.resourceBindings))
    {
    }

    ShaderReflectionData& ShaderReflectionData::operator=(const ShaderReflectionData& inOther) // NOLINT
    {
        vertexBindings = inOther.vertexBindings;
        resourceBindings = inOther.resourceBindings;
        return *this;
    }

    const RHI::PlatformVertexBinding& ShaderReflectionData::QueryVertexBindingChecked(const VertexSemantic& inSemantic) const
    {
        const auto iter = vertexBindings.find(inSemantic);
        Assert(iter != vertexBindings.end());
        return iter->second;
    }

    const ShaderReflectionData::LayoutAndResourceBinding& ShaderReflectionData::QueryResourceBindingChecked(const ResourceBindingName& inName) const
    {
        const auto iter = resourceBindings.find(inName);
        Assert(iter != resourceBindings.end());
        return iter->second;
    }

    void ShaderTypeRegistry::Register(const ShaderType* inShaderType)
    {
        const auto key = inShaderType->GetKey();
        Assert(!shaderContexts.contains(key));
        shaderContexts.emplace(std::make_pair(key, ShaderContext { inShaderType, shaderSourceHashNotCompiled }));
    }

    void ShaderTypeRegistry::Unregister(const ShaderType* inShaderType)
    {
        const auto key = inShaderType->GetKey();
        Assert(shaderContexts.contains(key));
        shaderContexts.erase(key);
    }

    void ShaderTypeRegistry::Reset(const ShaderType* inShaderType)
    {
        shaderContexts.at(inShaderType->GetKey()) = ShaderContext { inShaderType, shaderSourceHashNotCompiled };
    }

    ShaderInstance ShaderTypeRegistry::GetShaderInstance(const ShaderType* inShaderType, const ShaderVariantMap& inShaderVariants) const
    {
        const ShaderContext& context = shaderContexts.at(inShaderType->GetKey());
        const ShaderVariantKey variantKey = GetShaderVariantKey(inShaderType->GetVariantFields(), inShaderVariants);

        AssertWithReason(context.shaderModuleDatas.contains(variantKey), std::format("shader type {} is not compiled", inShaderType->GetName()));
        const ShaderModuleData& shaderModuleData = context.shaderModuleDatas.at(variantKey);

        if (!context.shaderModules.contains(variantKey)) {
            // TODO
        }
        // TODO
    }

    ShaderVariantKey ShaderTypeRegistry::GetShaderVariantKey(const ShaderVariantFieldVec& inFields, const ShaderVariantMap& inShaderVariants)
    {
        // TODO
        return 0;
    }
}

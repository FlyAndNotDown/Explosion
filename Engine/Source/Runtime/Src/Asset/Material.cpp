//
// Created by johnk on 2025/3/21.
//

#include <Runtime/Asset/Material.h>

namespace Runtime {
    MaterialBoolVariantField::MaterialBoolVariantField()
        : defaultValue(false)
        , sortPriority(0)
    {
    }

    MaterialRangedUintVariantField::MaterialRangedUintVariantField()
        : defaultValue(0)
        , range(0, 0)
        , sortPriority(0)
    {
    }

    MaterialBoolParameter::MaterialBoolParameter()
        : defaultValue(false)
        , sortPriority(0)
    {
    }

    MaterialIntParameter::MaterialIntParameter()
        : defaultValue(false)
        , sortPriority(0)
    {
    }

    MaterialFloatParameter::MaterialFloatParameter()
        : defaultValue(0.0f)
        , sortPriority(0)
    {
    }

    MaterialFVec2Parameter::MaterialFVec2Parameter()
        : defaultValue(Common::FVec2Consts::zero)
        , sortPriority(0)
    {
    }

    MaterialFVec3Parameter::MaterialFVec3Parameter()
        : defaultValue(Common::FVec3Consts::zero)
        , sortPriority(0)
    {
    }

    MaterialFVec4Parameter::MaterialFVec4Parameter()
        : defaultValue(Common::FVec4Consts::zero)
        , sortPriority(0)
    {
    }

    MaterialFMat4x4Parameter::MaterialFMat4x4Parameter()
        : defaultValue(Common::FMat4x4Consts::identity)
        , sortPriority(0)
    {
    }

    MaterialTextureParameter::MaterialTextureParameter()
        : sortPriority(0)
    {
    }

    MaterialRenderTargetParameter::MaterialRenderTargetParameter()
        : sortPriority(0)
    {
    }

    Material::Material(Core::Uri inUri)
        : Asset(std::move(inUri))
        , type(MaterialType::max)
    {
    }

    MaterialInstance::MaterialInstance(Core::Uri inUri)
        : Asset(std::move(inUri))
    {
    }
}

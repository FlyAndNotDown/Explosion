//
// Created by johnk on 19/2/2022.
//

#include <RHI/Sampler.h>

namespace RHI {
    SamplerCreateInfo::SamplerCreateInfo()
        : addressModeU(AddressMode::clampToEdge)
        , addressModeV(AddressMode::clampToEdge)
        , addressModeW(AddressMode::clampToEdge)
        , magFilter(FilterMode::nearest)
        , minFilter(FilterMode::nearest)
        , mipFilter(FilterMode::nearest)
        , lodMinClamp(0.0f)
        , lodMaxClamp(32.0f)
        , comparisonFunc(CompareFunc::never)
        , maxAnisotropy(1)
    {
    }

    SamplerCreateInfo& SamplerCreateInfo::SetAddressModeU(const AddressMode inMode)
    {
        addressModeU = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetAddressModeV(const AddressMode inMode)
    {
        addressModeV = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetAddressModeW(const AddressMode inMode)
    {
        addressModeW = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetMagFilter(const FilterMode inMode)
    {
        magFilter = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetMinFilter(const FilterMode inMode)
    {
        minFilter = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetMipFilter(const FilterMode inMode)
    {
        mipFilter = inMode;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetLodMinClamp(const float inValue)
    {
        lodMinClamp = inValue;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetLodMaxClamp(const float inValue)
    {
        lodMaxClamp = inValue;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetComparisonFunc(const CompareFunc inFunc)
    {
        comparisonFunc = inFunc;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetMaxAnisotropy(const uint8_t inValue)
    {
        maxAnisotropy = inValue;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::SetDebugName(std::string inName)
    {
        debugName = std::move(inName);
        return *this;
    }

    Sampler::Sampler(const SamplerCreateInfo&) {}

    Sampler::~Sampler() = default;
}

//
// Created by johnk on 5/3/2022.
//

#include <RHI/DirectX12/Common.h>
#include <RHI/DirectX12/Device.h>
#include <RHI/DirectX12/Sampler.h>

namespace RHI::DirectX12::Internal {
    static D3D12_FILTER_TYPE GetNativeFilterType(const FilterMode inFilterMode)
    {
        return inFilterMode == FilterMode::linear ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
    }

    static D3D12_FILTER GetNativeFilter(const SamplerCreateInfo& inCreateInfo)
    {
        const D3D12_FILTER_REDUCTION_TYPE reductionType = inCreateInfo.comparisonFunc == CompareFunc::never
            ? D3D12_FILTER_REDUCTION_TYPE_STANDARD
            : D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
        if (inCreateInfo.maxAnisotropy > 1) {
            return D3D12_ENCODE_ANISOTROPIC_FILTER(reductionType);
        }
        return D3D12_ENCODE_BASIC_FILTER(GetNativeFilterType(inCreateInfo.minFilter), GetNativeFilterType(inCreateInfo.magFilter), GetNativeFilterType(inCreateInfo.mipFilter), reductionType);
    }
}

namespace RHI::DirectX12 {
    DX12Sampler::DX12Sampler(DX12Device& inDevice, const SamplerCreateInfo& inCreateInfo)
        : Sampler(inCreateInfo)
    {
        CreateDX12Descriptor(inDevice, inCreateInfo);
    }

    DX12Sampler::~DX12Sampler() = default;

    CD3DX12_CPU_DESCRIPTOR_HANDLE DX12Sampler::GetNativeCpuDescriptorHandle() const
    {
        return descriptorAllocation->GetCpuHandle();
    }

    void DX12Sampler::CreateDX12Descriptor(DX12Device& inDevice, const SamplerCreateInfo& inCreateInfo) // NOLINT
    {
        D3D12_SAMPLER_DESC desc {};
        desc.AddressU = EnumCast<AddressMode, D3D12_TEXTURE_ADDRESS_MODE>(inCreateInfo.addressModeU);
        desc.AddressV = EnumCast<AddressMode, D3D12_TEXTURE_ADDRESS_MODE>(inCreateInfo.addressModeV);
        desc.AddressW = EnumCast<AddressMode, D3D12_TEXTURE_ADDRESS_MODE>(inCreateInfo.addressModeW);
        desc.Filter = Internal::GetNativeFilter(inCreateInfo);
        desc.MinLOD = inCreateInfo.lodMinClamp;
        desc.MaxLOD = inCreateInfo.lodMaxClamp;
        desc.ComparisonFunc = EnumCast<CompareFunc, D3D12_COMPARISON_FUNC>(inCreateInfo.comparisonFunc);
        desc.MaxAnisotropy = inCreateInfo.maxAnisotropy;

        descriptorAllocation = inDevice.AllocateSamplerDescriptor();
        inDevice.GetNative()->CreateSampler(&desc, descriptorAllocation->GetCpuHandle());
    }
}

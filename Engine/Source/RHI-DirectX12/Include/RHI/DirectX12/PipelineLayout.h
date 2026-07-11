//
// Created by johnk on 11/3/2022.
//

#pragma once

#include <unordered_map>
#include <optional>
#include <vector>

#include <wrl/client.h>

#include <RHI/PipelineLayout.h>

using Microsoft::WRL::ComPtr;

namespace RHI::DirectX12 {
    class DX12Device;

    struct RootParameterKey {
        uint8_t layoutIndex;
        HlslBinding binding;
        ShaderStageFlags shaderVisibility;

        bool operator==(const RootParameterKey& inOther) const;
    };

    struct RootParameterKeyHashProvider {
        size_t operator()(const RootParameterKey& inKey) const;
    };

    using RootParameterIndex = uint32_t;
    using BindingTypeAndRootParameterIndex = std::pair<BindingType, uint32_t>;
    using RootParameterIndexMap = std::unordered_map<RootParameterKey, BindingTypeAndRootParameterIndex, RootParameterKeyHashProvider>;

    class DX12PipelineLayout final : public PipelineLayout {
    public:
        NonCopyable(DX12PipelineLayout)
        DX12PipelineLayout(DX12Device& inDevice, const PipelineLayoutCreateInfo& inCreateInfo);
        ~DX12PipelineLayout() override;

        std::optional<BindingTypeAndRootParameterIndex> QueryRootDescriptorParameterIndex(
            uint8_t inLayoutIndex,
            const HlslBinding& inBinding,
            ShaderStageFlags inShaderVisibility);
        RootParameterIndex QueryRootConstantParameterIndex(uint32_t inPipelineConstantIndex) const;
        ID3D12RootSignature* GetNative() const;

    private:
        void CreateNativeRootSignature(DX12Device& inDevice, const PipelineLayoutCreateInfo& inCreateInfo);

        ComPtr<ID3D12RootSignature> nativeRootSignature;
        RootParameterIndexMap rootParameterIndexMap;
        std::vector<RootParameterIndex> rootConstantParameterIndices;
    };
}

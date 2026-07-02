//
// Created by johnk on 2026/7/2.
//

#pragma once

#include <vector>

#include <wrl/client.h>
#include <directx/d3d12.h>

#include <RHI/PipelineCache.h>

using Microsoft::WRL::ComPtr;

namespace RHI::DirectX12 {
    class DX12Device;

    class DX12PipelineCache final : public PipelineCache {
    public:
        NonCopyable(DX12PipelineCache)
        DX12PipelineCache(DX12Device& inDevice, const PipelineCacheCreateInfo& inCreateInfo);
        ~DX12PipelineCache() override;

        std::vector<uint8_t> GetData() override;
        ID3D12PipelineLibrary* GetNative() const;

    private:
        void CreateNativePipelineLibrary(DX12Device& inDevice, const PipelineCacheCreateInfo& inCreateInfo);

        std::vector<uint8_t> initialData;
        ComPtr<ID3D12PipelineLibrary> nativePipelineLibrary;
    };
}

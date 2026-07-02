//
// Created by johnk on 2026/7/2.
//

#include <cstring>

#include <RHI/DirectX12/PipelineCache.h>
#include <RHI/DirectX12/Device.h>
#include <RHI/DirectX12/Common.h>

namespace RHI::DirectX12 {
    DX12PipelineCache::DX12PipelineCache(DX12Device& inDevice, const PipelineCacheCreateInfo& inCreateInfo)
        : PipelineCache(inCreateInfo)
    {
        CreateNativePipelineLibrary(inDevice, inCreateInfo);
    }

    DX12PipelineCache::~DX12PipelineCache() = default;

    std::vector<uint8_t> DX12PipelineCache::GetData()
    {
        if (nativePipelineLibrary == nullptr) {
            return {};
        }

        const SIZE_T size = nativePipelineLibrary->GetSerializedSize();
        std::vector<uint8_t> result(size);
        Assert(SUCCEEDED(nativePipelineLibrary->Serialize(result.data(), size)));
        return result;
    }

    ID3D12PipelineLibrary* DX12PipelineCache::GetNative() const
    {
        return nativePipelineLibrary.Get();
    }

    void DX12PipelineCache::CreateNativePipelineLibrary(DX12Device& inDevice, const PipelineCacheCreateInfo& inCreateInfo)
    {
        ComPtr<ID3D12Device1> nativeDevice1;
        if (FAILED(inDevice.GetNative()->QueryInterface(IID_PPV_ARGS(&nativeDevice1)))) {
            return;
        }

        // CreatePipelineLibrary does not copy the blob, so it must outlive the library object.
        if (inCreateInfo.initialDataSize > 0) {
            initialData.resize(inCreateInfo.initialDataSize);
            memcpy(initialData.data(), inCreateInfo.initialData, inCreateInfo.initialDataSize);
        }

        // A stale or driver-incompatible blob makes CreatePipelineLibrary fail; fall back to an empty library then.
        if (FAILED(nativeDevice1->CreatePipelineLibrary(initialData.data(), initialData.size(), IID_PPV_ARGS(&nativePipelineLibrary)))) {
            initialData.clear();
            Assert(SUCCEEDED(nativeDevice1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&nativePipelineLibrary))));
        }
    }
}

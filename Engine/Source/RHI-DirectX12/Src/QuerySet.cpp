//
// Created by johnk on 2026/6/30.
//

#include <RHI/DirectX12/QuerySet.h>
#include <RHI/DirectX12/Device.h>
#include <RHI/DirectX12/Common.h>
#include <Common/Debug.h>

namespace RHI::DirectX12 {
    DX12QuerySet::DX12QuerySet(DX12Device& inDevice, const QuerySetCreateInfo& inCreateInfo)
        : QuerySet(inCreateInfo)
    {
        CreateNativeQueryHeap(inDevice, inCreateInfo);
    }

    DX12QuerySet::~DX12QuerySet() = default;

    ID3D12QueryHeap* DX12QuerySet::GetNative() const
    {
        return nativeQueryHeap.Get();
    }

    void DX12QuerySet::CreateNativeQueryHeap(DX12Device& inDevice, const QuerySetCreateInfo& inCreateInfo)
    {
        D3D12_QUERY_HEAP_DESC desc = {};
        desc.Type = EnumCast<QueryType, D3D12_QUERY_HEAP_TYPE>(inCreateInfo.type);
        desc.Count = inCreateInfo.count;
        desc.NodeMask = 0;

        Assert(SUCCEEDED(inDevice.GetNative()->CreateQueryHeap(&desc, IID_PPV_ARGS(&nativeQueryHeap))));
    }
}

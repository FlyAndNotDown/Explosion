//
// Created by johnk on 2026/6/30.
//

#pragma once

#include <wrl/client.h>
#include <directx/d3d12.h>

#include <RHI/QuerySet.h>

using Microsoft::WRL::ComPtr;

namespace RHI::DirectX12 {
    class DX12Device;

    class DX12QuerySet final : public QuerySet {
    public:
        NonCopyable(DX12QuerySet)
        DX12QuerySet(DX12Device& inDevice, const QuerySetCreateInfo& inCreateInfo);
        ~DX12QuerySet() override;

        ID3D12QueryHeap* GetNative() const;

    private:
        void CreateNativeQueryHeap(DX12Device& inDevice, const QuerySetCreateInfo& inCreateInfo);

        ComPtr<ID3D12QueryHeap> nativeQueryHeap;
    };
}

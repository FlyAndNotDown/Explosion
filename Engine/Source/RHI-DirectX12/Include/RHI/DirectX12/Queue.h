//
// Created by johnk on 15/1/2022.
//

#pragma once

#include <wrl/client.h>
#include <directx/d3d12.h>

#include <RHI/Queue.h>

using Microsoft::WRL::ComPtr;

namespace RHI::DirectX12 {
    class DX12Queue final : public Queue {
    public:
        NonCopyable(DX12Queue)
        DX12Queue(QueueType inType, ComPtr<ID3D12CommandQueue>&& inNativeCmdQueue);
        ~DX12Queue() override;

        void Flush(Fence* inFenceToSignal) override;
        float GetTimestampPeriod() override;

        ID3D12CommandQueue* GetNative() const;

    private:
        void SubmitInternal(CommandBuffer* inCmdBuffer, const QueueSubmitInfo& inSubmitInfo) override;

        ComPtr<ID3D12CommandQueue> nativeCmdQueue;
    };
}

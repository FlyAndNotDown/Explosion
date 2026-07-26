//
// Created by johnk on 2023/3/21.
//

#include <RHI/Dummy/Queue.h>

namespace RHI::Dummy {
    DummyQueue::DummyQueue()
        : Queue(QueueType::graphics)
    {
    }

    DummyQueue::~DummyQueue() = default;

    void DummyQueue::SubmitInternal(RHI::CommandBuffer* commandBuffer, const QueueSubmitInfo& submitInfo)
    {
    }

    void DummyQueue::Flush(RHI::Fence* fenceToSignal)
    {
    }

    float DummyQueue::GetTimestampPeriod()
    {
        return 1.0f;
    }
}

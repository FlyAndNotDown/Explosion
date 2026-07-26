//
// Created by johnk on 2023/3/21.
//

#include <RHI/Dummy/CommandBuffer.h>
#include <RHI/Dummy/CommandRecorder.h>

namespace RHI::Dummy {
    DummyCommandBuffer::DummyCommandBuffer(const QueueType inQueueType)
        : CommandBuffer(inQueueType)
    {
    }

    Common::UniquePtr<CommandRecorder> DummyCommandBuffer::Begin()
    {
        return { new DummyCommandRecorder(*this) };
    }
}

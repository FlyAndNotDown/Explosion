//
// Created by johnk on 21/2/2022.
//

#include <RHI/CommandBuffer.h>

namespace RHI {
    CommandBuffer::CommandBuffer(const QueueType inQueueType)
        : queueType(inQueueType)
    {
    }

    CommandBuffer::~CommandBuffer() = default;

    QueueType CommandBuffer::GetQueueType() const
    {
        return queueType;
    }
}

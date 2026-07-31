//
// Created by johnk on 15/1/2022.
//

#include <RHI/Queue.h>
#include <RHI/CommandBuffer.h>
#include <Common/Debug.h>

namespace RHI {
    QueueSubmitInfo::QueueSubmitInfo()
        : signalFence(nullptr)
    {
    }

    QueueSubmitInfo& QueueSubmitInfo::AddWaitSemaphore(Semaphore* inSemaphore)
    {
        waitSemaphores.emplace_back(inSemaphore);
        return *this;
    }

    QueueSubmitInfo& QueueSubmitInfo::AddSignalSemaphore(Semaphore* inSemaphore)
    {
        signalSemaphores.emplace_back(inSemaphore);
        return *this;
    }

    QueueSubmitInfo& QueueSubmitInfo::SetSignalFence(Fence* inSignalFence)
    {
        signalFence = inSignalFence;
        return *this;
    }

    QueueSubmitInfo& QueueSubmitInfo::SetWaitSemaphores(const std::vector<Semaphore*>& inSemaphores)
    {
        waitSemaphores = inSemaphores;
        return *this;
    }

    QueueSubmitInfo& QueueSubmitInfo::SetSignalSemaphores(const std::vector<Semaphore*>& inSemaphores)
    {
        signalSemaphores = inSemaphores;
        return *this;
    }

    Queue::Queue(const QueueType inType)
        : type(inType)
    {
    }

    Queue::~Queue() = default;

    QueueType Queue::GetType() const
    {
        return type;
    }

    void Queue::Submit(CommandBuffer* commandBuffer, const QueueSubmitInfo& submitInfo)
    {
        Assert(commandBuffer != nullptr);
        Assert(commandBuffer->GetQueueType() == type);
        SubmitInternal(commandBuffer, submitInfo);
    }
}

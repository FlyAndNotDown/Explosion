//
// Created by johnk on 16/1/2022.
//

#pragma once

#include <memory>
#include <mutex>

#include <vulkan/vulkan.h>

#include <RHI/Queue.h>
#include <RHI/Synchronous.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanQueue final : public Queue {
    public:
        NonCopyable(VulkanQueue)
        VulkanQueue(VulkanDevice& inDevice, QueueType inType, uint32_t inFamilyIndex, VkQueue inNativeQueue, std::shared_ptr<std::mutex> inNativeQueueMutex);
        ~VulkanQueue() override;

        void Flush(Fence* inFenceToSignal) override;
        float GetTimestampPeriod() override;

        uint32_t GetFamilyIndex() const;
        VkQueue GetNative() const;
        VkResult Present(const VkPresentInfoKHR& inPresentInfo) const;
        void WaitIdle() const;

    private:
        void SubmitInternal(CommandBuffer* inCmdBuffer, const QueueSubmitInfo& inSubmitInfo) override;

        VulkanDevice& device;
        uint32_t familyIndex;
        VkQueue nativeQueue;
        std::shared_ptr<std::mutex> nativeQueueMutex;
    };
}

//
// Created by johnk on 16/1/2022.
//

#pragma once

#include <vulkan/vulkan.h>

#include <RHI/Queue.h>
#include <RHI/Synchronous.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanQueue final : public Queue {
    public:
        NonCopyable(VulkanQueue)
        VulkanQueue(VulkanDevice& inDevice, QueueType inType, VkQueue inNativeQueue);
        ~VulkanQueue() override;

        void Flush(Fence* inFenceToSignal) override;
        float GetTimestampPeriod() override;

        VkQueue GetNative() const;

    private:
        void SubmitInternal(CommandBuffer* inCmdBuffer, const QueueSubmitInfo& inSubmitInfo) override;

        VulkanDevice& device;
        VkQueue nativeQueue;
    };
}

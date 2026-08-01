//
// Created by johnk on 2022/1/26.
//

#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <RHI/Buffer.h>
#include <Common/Utility.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanBuffer final : public Buffer {
    public:
        NonCopyable(VulkanBuffer)
        VulkanBuffer(VulkanDevice& inDevice, const BufferCreateInfo& inCreateInfo);
        ~VulkanBuffer() override;

        void* Map(MapMode inMapMode, size_t inOffset, size_t inLength) override;
        void Unmap() override;

        VkBuffer GetNative() const;
        BufferUsageFlags GetUsages() const;

    private:
        Common::UniquePtr<BufferView> CreateBufferViewInternal(const BufferViewCreateInfo& inCreateInfo) override;
        void CreateNativeBuffer(const BufferCreateInfo& inCreateInfo);
        void TransitionToInitState(const BufferCreateInfo& inCreateInfo);

        VulkanDevice& device;
        VkBuffer nativeBuffer;
        VmaAllocation nativeAllocation;
        BufferUsageFlags usages;
        MapMode mapMode;
        size_t mapOffset;
        size_t mapLength;
    };
}

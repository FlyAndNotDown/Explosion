//
// Created by Zach Lee on 2022/3/7.
//

#include <RHI/Vulkan/Texture.h>
#include <RHI/Vulkan/TextureView.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Common.h>
#include <RHI/Vulkan/Queue.h>
#include <RHI/Vulkan/CommandBuffer.h>
#include <RHI/Vulkan/CommandRecorder.h>
#include <RHI/Vulkan/Synchronous.h>

namespace RHI::Vulkan::Internal {
    static VkImageCreateFlags GetNativeImageCreateFlags(TextureType inType)
    {
        return inType == TextureType::tCube || inType == TextureType::tCubeArray ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    }
}

namespace RHI::Vulkan {
    VulkanTexture::VulkanTexture(VulkanDevice& inDevice, const TextureCreateInfo& inCreateInfo, VkImage inNativeImage)
        : Texture(inCreateInfo)
        , device(inDevice)
        , nativeImage(inNativeImage)
        , nativeAspect(EnumCast<TextureAspect, VkImageAspectFlags>(GetTextureAspect(inCreateInfo.format)))
        , ownMemory(false)
    {
    }

    VulkanTexture::VulkanTexture(VulkanDevice& inDevice, const TextureCreateInfo& inCreateInfo)
        : Texture(inCreateInfo)
        , device(inDevice)
        , nativeImage(VK_NULL_HANDLE)
        , nativeAspect(EnumCast<TextureAspect, VkImageAspectFlags>(GetTextureAspect(inCreateInfo.format)))
        , ownMemory(true)
    {
        CreateNativeImage(inCreateInfo);
        TransitionToInitState(inCreateInfo);
    }

    VulkanTexture::~VulkanTexture()
    {
        if (nativeImage != VK_NULL_HANDLE && ownMemory) {
            vmaDestroyImage(device.GetNativeAllocator(), nativeImage, nativeAllocation);
        }
    }

    Common::UniquePtr<TextureView> VulkanTexture::CreateTextureViewInternal(const TextureViewCreateInfo& inCreateInfo)
    {
        return Common::UniquePtr<TextureView>(new VulkanTextureView(*this, device, inCreateInfo));
    }

    VkImage VulkanTexture::GetNative() const
    {
        return nativeImage;
    }

    VkImageSubresourceRange VulkanTexture::GetNativeSubResourceFullRange() const
    {
        if (createInfo.type == TextureType::t3D) {
            return { nativeAspect, 0, createInfo.mipLevels, 0, 1 };
        } else {
            return { nativeAspect, 0, createInfo.mipLevels, 0, createInfo.depthOrArraySize };
        }
    }

    void VulkanTexture::CreateNativeImage(const TextureCreateInfo& inCreateInfo)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = Internal::GetNativeImageCreateFlags(inCreateInfo.type);
        imageInfo.mipLevels = inCreateInfo.mipLevels;
        if (inCreateInfo.type == TextureType::t3D) {
            imageInfo.extent = { inCreateInfo.width, inCreateInfo.height, inCreateInfo.depthOrArraySize };
            imageInfo.arrayLayers = 1;
        } else {
            imageInfo.extent = { inCreateInfo.width, inCreateInfo.height, 1 };
            imageInfo.arrayLayers = inCreateInfo.depthOrArraySize;
        }
        imageInfo.samples = static_cast<VkSampleCountFlagBits>(inCreateInfo.samples);
        imageInfo.imageType = EnumCast<TextureType, VkImageType>(inCreateInfo.type);
        imageInfo.format = EnumCast<PixelFormat, VkFormat>(inCreateInfo.format);
        imageInfo.usage = FlagsCast<TextureUsageFlags, VkImageUsageFlags>(inCreateInfo.usages);

        const auto& queueFamilyIndices = device.GetActiveQueueFamilyIndices();
        if (queueFamilyIndices.size() > 1) {
            imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

        Assert(vmaCreateImage(device.GetNativeAllocator(), &imageInfo, &allocInfo, &nativeImage, &nativeAllocation, nullptr) == VK_SUCCESS);

#if BUILD_CONFIG_DEBUG
        if (!inCreateInfo.debugName.empty()) {
            device.SetObjectName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(nativeImage), inCreateInfo.debugName.c_str());
        }
#endif
    }

    void VulkanTexture::TransitionToInitState(const TextureCreateInfo& inCreateInfo)
    {
        if (inCreateInfo.initialState > TextureState::undefined) {
            Queue* queue = device.GetQueue(QueueType::graphics, 0);
            Assert(queue);

            const auto fence = device.CreateFence(false);
            const auto commandBuffer = device.CreateCommandBuffer(QueueType::graphics);
            const auto commandRecorder = commandBuffer->Begin();
            commandRecorder->ResourceBarrier(Barrier::Transition(this, TextureState::undefined, inCreateInfo.initialState));
            commandRecorder->End();

            QueueSubmitInfo submitInfo {};
            submitInfo.signalFence = fence.Get();
            queue->Submit(commandBuffer.Get(), submitInfo);
            fence->Wait();
        }
    }
}

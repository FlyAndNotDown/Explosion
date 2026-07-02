//
// Created by johnk on 2026/7/2.
//

#include <RHI/Vulkan/PipelineCache.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Common.h>
#include <Common/Debug.h>

namespace RHI::Vulkan {
    VulkanPipelineCache::VulkanPipelineCache(VulkanDevice& inDevice, const PipelineCacheCreateInfo& inCreateInfo)
        : PipelineCache(inCreateInfo)
        , device(inDevice)
        , nativePipelineCache(VK_NULL_HANDLE)
    {
        CreateNativePipelineCache(inCreateInfo);
    }

    VulkanPipelineCache::~VulkanPipelineCache()
    {
        if (nativePipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(device.GetNative(), nativePipelineCache, nullptr);
        }
    }

    std::vector<uint8_t> VulkanPipelineCache::GetData()
    {
        size_t size = 0;
        Assert(vkGetPipelineCacheData(device.GetNative(), nativePipelineCache, &size, nullptr) == VK_SUCCESS);

        std::vector<uint8_t> result(size);
        Assert(vkGetPipelineCacheData(device.GetNative(), nativePipelineCache, &size, result.data()) == VK_SUCCESS);
        return result;
    }

    VkPipelineCache VulkanPipelineCache::GetNative() const
    {
        return nativePipelineCache;
    }

    void VulkanPipelineCache::CreateNativePipelineCache(const PipelineCacheCreateInfo& inCreateInfo)
    {
        VkPipelineCacheCreateInfo cacheInfo = {};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheInfo.initialDataSize = inCreateInfo.initialDataSize;
        cacheInfo.pInitialData = inCreateInfo.initialData;

        Assert(vkCreatePipelineCache(device.GetNative(), &cacheInfo, nullptr, &nativePipelineCache) == VK_SUCCESS);

#if BUILD_CONFIG_DEBUG
        if (!inCreateInfo.debugName.empty()) {
            device.SetObjectName(VK_OBJECT_TYPE_PIPELINE_CACHE, reinterpret_cast<uint64_t>(nativePipelineCache), inCreateInfo.debugName.c_str());
        }
#endif
    }
}

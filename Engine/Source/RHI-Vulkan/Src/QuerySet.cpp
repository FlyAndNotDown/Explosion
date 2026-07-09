//
// Created by johnk on 2026/6/30.
//

#include <RHI/Vulkan/QuerySet.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Common.h>
#include <Common/Debug.h>

namespace RHI::Vulkan {
    VulkanQuerySet::VulkanQuerySet(VulkanDevice& inDevice, const QuerySetCreateInfo& inCreateInfo)
        : QuerySet(inCreateInfo)
        , device(inDevice)
        , nativeQueryPool(VK_NULL_HANDLE)
    {
        CreateNativeQueryPool(inCreateInfo);
    }

    VulkanQuerySet::~VulkanQuerySet()
    {
        vkDestroyQueryPool(device.GetNative(), nativeQueryPool, nullptr);
    }

    VkQueryPool VulkanQuerySet::GetNative() const
    {
        return nativeQueryPool;
    }

    void VulkanQuerySet::CreateNativeQueryPool(const QuerySetCreateInfo& inCreateInfo)
    {
        VkQueryPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        poolInfo.queryType = EnumCast<QueryType, VkQueryType>(inCreateInfo.type);
        poolInfo.queryCount = inCreateInfo.count;

        Assert(vkCreateQueryPool(device.GetNative(), &poolInfo, nullptr, &nativeQueryPool) == VK_SUCCESS);
    }
}

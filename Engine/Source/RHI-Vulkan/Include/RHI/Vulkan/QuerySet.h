//
// Created by johnk on 2026/6/30.
//

#pragma once

#include <vulkan/vulkan.h>

#include <RHI/QuerySet.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanQuerySet final : public QuerySet {
    public:
        NonCopyable(VulkanQuerySet)
        VulkanQuerySet(VulkanDevice& inDevice, const QuerySetCreateInfo& inCreateInfo);
        ~VulkanQuerySet() override;

        VkQueryPool GetNative() const;

    private:
        void CreateNativeQueryPool(const QuerySetCreateInfo& inCreateInfo);

        VulkanDevice& device;
        VkQueryPool nativeQueryPool;
    };
}

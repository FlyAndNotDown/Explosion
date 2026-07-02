//
// Created by johnk on 2026/7/2.
//

#pragma once

#include <vulkan/vulkan.h>

#include <RHI/PipelineCache.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanPipelineCache final : public PipelineCache {
    public:
        NonCopyable(VulkanPipelineCache)
        VulkanPipelineCache(VulkanDevice& inDevice, const PipelineCacheCreateInfo& inCreateInfo);
        ~VulkanPipelineCache() override;

        std::vector<uint8_t> GetData() override;
        VkPipelineCache GetNative() const;

    private:
        void CreateNativePipelineCache(const PipelineCacheCreateInfo& inCreateInfo);

        VulkanDevice& device;
        VkPipelineCache nativePipelineCache;
    };
}

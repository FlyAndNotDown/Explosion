//
// Created by Zach Lee on 2022/4/2.
//

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <RHI/PipelineLayout.h>

namespace RHI::Vulkan {
    class VulkanDevice;

    class VulkanPipelineLayout final : public PipelineLayout {
    public:
        NonCopyable(VulkanPipelineLayout)
        VulkanPipelineLayout(VulkanDevice& inDevice, const PipelineLayoutCreateInfo& inCreateInfo);
        ~VulkanPipelineLayout() override;

        VkPipelineLayout GetNative() const;
        const VkPushConstantRange& GetPushConstantRange(uint32_t inPipelineConstantIndex) const;

    private:
        void CreateNativePipelineLayout(const PipelineLayoutCreateInfo& inCreateInfo);

        VulkanDevice& device;
        VkPipelineLayout nativePipelineLayout;
        std::vector<VkPushConstantRange> pushConstantRanges;
    };
}
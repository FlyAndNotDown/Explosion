//
// Created by Zach Lee on 2022/4/2.
//
#include <RHI/Vulkan/PipelineLayout.h>
#include <RHI/Vulkan/BindGroupLayout.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Common.h>

namespace RHI::Vulkan {

    VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& inDevice, const PipelineLayoutCreateInfo& inCreateInfo)
        : PipelineLayout(inCreateInfo)
        , device(inDevice)
        , nativePipelineLayout(VK_NULL_HANDLE)
    {
        CreateNativePipelineLayout(inCreateInfo);
    }

    VulkanPipelineLayout::~VulkanPipelineLayout()
    {
        if (nativePipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.GetNative(), nativePipelineLayout, nullptr);
        }
    }

    VkPipelineLayout VulkanPipelineLayout::GetNative() const
    {
        return nativePipelineLayout;
    }

    const VkPushConstantRange& VulkanPipelineLayout::GetPushConstantRange(const uint32_t inPipelineConstantIndex) const
    {
        return pushConstantRanges[inPipelineConstantIndex];
    }

    void VulkanPipelineLayout::CreateNativePipelineLayout(const PipelineLayoutCreateInfo& inCreateInfo)
    {
        std::vector<VkDescriptorSetLayout> setLayouts;
        for (const auto* bindGroupLayout : inCreateInfo.bindGroupLayouts) {
            const auto layoutIndex = bindGroupLayout->GetLayoutIndex();
            if (setLayouts.size() <= layoutIndex) {
                setLayouts.resize(layoutIndex + 1, VK_NULL_HANDLE);
            }

            AssertWithReason(setLayouts[layoutIndex] == VK_NULL_HANDLE, "pipeline layouts cannot contain duplicate bind group layout indices");
            setLayouts[layoutIndex] = static_cast<const VulkanBindGroupLayout*>(bindGroupLayout)->GetNative();
        }

        VkDescriptorSetLayout emptySetLayout = VK_NULL_HANDLE;
        for (auto& setLayout : setLayouts) {
            if (setLayout != VK_NULL_HANDLE) {
                continue;
            }
            if (emptySetLayout == VK_NULL_HANDLE) {
                VkDescriptorSetLayoutCreateInfo emptySetLayoutInfo = {};
                emptySetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                Assert(vkCreateDescriptorSetLayout(device.GetNative(), &emptySetLayoutInfo, nullptr, &emptySetLayout) == VK_SUCCESS);
            }
            setLayout = emptySetLayout;
        }

        pushConstantRanges.resize(inCreateInfo.pipelineConstantLayouts.size());
        for (uint32_t i = 0; i < inCreateInfo.pipelineConstantLayouts.size(); ++i) {
            const auto& constantInfo = inCreateInfo.pipelineConstantLayouts[i];
            pushConstantRanges[i].stageFlags = FlagsCast<ShaderStageFlags, VkShaderStageFlags>(constantInfo.stageFlags);
            pushConstantRanges[i].offset = std::get<GlslPipelineConstantBinding>(constantInfo.platformBinding).offset;
            pushConstantRanges[i].size = constantInfo.size;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = setLayouts.size();
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        Assert(vkCreatePipelineLayout(device.GetNative(), &pipelineLayoutInfo, nullptr, &nativePipelineLayout) == VK_SUCCESS);

        if (emptySetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device.GetNative(), emptySetLayout, nullptr);
        }

#if BUILD_CONFIG_DEBUG
        if (!inCreateInfo.debugName.empty()) {
            device.SetObjectName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(nativePipelineLayout), inCreateInfo.debugName.c_str());
        }
#endif
    }

}

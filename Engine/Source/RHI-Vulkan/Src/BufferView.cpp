//
// Created by swtpotato on 2022/8/2.
//

#include <RHI/Vulkan/Buffer.h>
#include <RHI/Vulkan/BufferView.h>

namespace RHI::Vulkan {
    VulkanBufferView::VulkanBufferView(VulkanBuffer& inBuffer, const BufferViewCreateInfo& inCreateInfo)
        : BufferView(inCreateInfo)
        , buffer(inBuffer)
    {
    }

    VulkanBufferView::~VulkanBufferView() = default;

    VulkanBuffer& VulkanBufferView::GetBuffer() const
    {
        return buffer;
    }
}

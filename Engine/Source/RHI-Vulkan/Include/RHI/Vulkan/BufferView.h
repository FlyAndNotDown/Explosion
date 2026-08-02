//
// Created by swtpotato on 2022/8/2.
//

#pragma once

#include <RHI/BufferView.h>

namespace RHI::Vulkan {
    class VulkanBuffer;
    class VulkanDevice;

    class VulkanBufferView final : public BufferView {
    public:
        NonCopyable(VulkanBufferView)
        VulkanBufferView(VulkanBuffer& inBuffer, const BufferViewCreateInfo& inCreateInfo);
        ~VulkanBufferView() override;

        VulkanBuffer& GetBuffer() const;

    private:
        VulkanBuffer& buffer;
    };
}

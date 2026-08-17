//
// Created by Zach Lee on 2022/6/4.
//

#include <RHI/Vulkan/CommandRecorder.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Gpu.h>
#include <RHI/Vulkan/Pipeline.h>
#include <RHI/Vulkan/CommandBuffer.h>
#include <RHI/Vulkan/Buffer.h>
#include <RHI/Vulkan/BufferView.h>
#include <RHI/Vulkan/TextureView.h>
#include <RHI/Vulkan/Texture.h>
#include <RHI/Vulkan/Common.h>
#include <RHI/Vulkan/Instance.h>
#include <RHI/Vulkan/BindGroup.h>
#include <RHI/Vulkan/PipelineLayout.h>
#include <RHI/Vulkan/QuerySet.h>
#include <RHI/Synchronous.h>

#include <algorithm>

namespace RHI::Vulkan {
    static VkAccessFlags GetBufferMemoryBarrierAccessFlags(const BufferState inState)
    {
        static std::unordered_map<BufferState, VkAccessFlags> map = {
            { BufferState::undefined, VK_ACCESS_NONE },
            { BufferState::staging, VK_ACCESS_HOST_WRITE_BIT },
            { BufferState::copySrc, VK_ACCESS_TRANSFER_READ_BIT },
            { BufferState::copyDst, VK_ACCESS_TRANSFER_WRITE_BIT },
            { BufferState::shaderReadOnly, VK_ACCESS_SHADER_READ_BIT },
            { BufferState::storage, VK_ACCESS_SHADER_READ_BIT },
            { BufferState::rwStorage, VK_ACCESS_SHADER_WRITE_BIT },
            { BufferState::indirect, VK_ACCESS_INDIRECT_COMMAND_READ_BIT }
        };
        return map.at(inState);
    }

    static VkPipelineStageFlags GetBufferPipelineBarrierSrcStage(const BufferState inState)
    {
        static std::unordered_map<BufferState, VkPipelineStageFlags> map = {
            { BufferState::undefined, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT },
            { BufferState::staging, VK_PIPELINE_STAGE_HOST_BIT },
            { BufferState::copySrc, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { BufferState::copyDst, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { BufferState::shaderReadOnly, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::storage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::rwStorage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::indirect, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT }
        };
        return map.at(inState);
    }

    static VkPipelineStageFlags GetBufferPipelineBarrierDstStage(const BufferState inState)
    {
        static std::unordered_map<BufferState, VkPipelineStageFlags> map = {
            { BufferState::undefined, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
            { BufferState::staging, VK_PIPELINE_STAGE_HOST_BIT },
            { BufferState::copySrc, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { BufferState::copyDst, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { BufferState::shaderReadOnly, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::storage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::rwStorage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { BufferState::indirect, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT }
        };
        return map.at(inState);
    }

    static VkAccessFlags GetTextureMemoryBarrierAccessFlags(const TextureState inState)
    {
        static std::unordered_map<TextureState, VkAccessFlags> map = {
            { TextureState::undefined, VK_ACCESS_NONE },
            { TextureState::copySrc, VK_ACCESS_TRANSFER_READ_BIT },
            { TextureState::copyDst, VK_ACCESS_TRANSFER_WRITE_BIT },
            { TextureState::shaderReadOnly, VK_ACCESS_SHADER_READ_BIT },
            { TextureState::renderTarget, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
            { TextureState::storage, VK_ACCESS_SHADER_READ_BIT },
            { TextureState::rwStorage, VK_ACCESS_SHADER_WRITE_BIT },
            { TextureState::depthStencilReadonly, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT },
            { TextureState::depthReadStencilWrite, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },
            { TextureState::depthWriteStencilRead, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },
            { TextureState::depthStencilWrite, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },
            { TextureState::present, VK_ACCESS_MEMORY_READ_BIT }
        };
        return map.at(inState);
    }

    static VkPipelineStageFlags GetTexturePipelineBarrierSrcStage(const TextureState inState)
    {
        static std::unordered_map<TextureState, VkPipelineStageFlags> map = {
            { TextureState::undefined, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT },
            { TextureState::copySrc, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { TextureState::copyDst, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { TextureState::shaderReadOnly, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::renderTarget, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { TextureState::storage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::rwStorage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::depthStencilReadonly, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthReadStencilWrite, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthWriteStencilRead, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthStencilWrite, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::present, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }
        };
        return map.at(inState);
    }

    static VkPipelineStageFlags GetTexturePipelineBarrierDstStage(const TextureState inState)
    {
        static std::unordered_map<TextureState, VkPipelineStageFlags> map = {
            { TextureState::undefined, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },
            { TextureState::copySrc, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { TextureState::copyDst, VK_PIPELINE_STAGE_TRANSFER_BIT },
            { TextureState::shaderReadOnly, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::renderTarget, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { TextureState::storage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::rwStorage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT },
            { TextureState::depthStencilReadonly, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthReadStencilWrite, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthWriteStencilRead, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::depthStencilWrite, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
            { TextureState::present, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }
        };
        return map.at(inState);
    }

    static VkImageLayout GetTextureLayout(const TextureState inState)
    {
        static std::unordered_map<TextureState, VkImageLayout> map = {
            { TextureState::undefined, VK_IMAGE_LAYOUT_UNDEFINED },
            { TextureState::copySrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
            { TextureState::copyDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
            { TextureState::shaderReadOnly, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { TextureState::renderTarget, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
            { TextureState::storage, VK_IMAGE_LAYOUT_GENERAL },
            { TextureState::rwStorage, VK_IMAGE_LAYOUT_GENERAL },
            { TextureState::depthStencilReadonly, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
            { TextureState::depthReadStencilWrite, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL },
            { TextureState::depthWriteStencilRead, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL },
            { TextureState::depthStencilWrite, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
            { TextureState::present, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
        };
        return map.at(inState);
    }

    static VkImageSubresourceLayers GetNativeImageSubResourceLayers(const TextureSubResourceInfo& subResourceInfo)
    {
        VkImageSubresourceLayers result {};
        result.mipLevel = subResourceInfo.mipLevel;
        result.baseArrayLayer = subResourceInfo.arrayLayer;
        result.layerCount = 1;
        result.aspectMask = EnumCast<TextureAspect, VkImageAspectFlags>(subResourceInfo.aspect);
        return result;
    }

    static VkBufferImageCopy GetNativeBufferImageCopy(const Texture& texture, const BufferTextureCopyInfo& copyInfo)
    {
        const auto bytesPerPixel = GetBytesPerPixel(texture.GetCreateInfo().format);

        VkBufferImageCopy result {};
        result.bufferOffset = copyInfo.bufferOffset;
        result.bufferRowLength = static_cast<uint32_t>(copyInfo.bufferRowPitch / bytesPerPixel);
        result.bufferImageHeight = static_cast<uint32_t>(copyInfo.bufferSlicePitch / copyInfo.bufferRowPitch);
        result.imageOffset = { static_cast<int32_t>(copyInfo.textureOrigin.x), static_cast<int32_t>(copyInfo.textureOrigin.y), static_cast<int32_t>(copyInfo.textureOrigin.z) };
        result.imageExtent = { copyInfo.copyRegion.x, copyInfo.copyRegion.y, copyInfo.copyRegion.z };
        result.imageSubresource = GetNativeImageSubResourceLayers(copyInfo.textureSubResource);
        return result;
    }
}

namespace RHI::Vulkan {
    VulkanCommandRecorder::VulkanCommandRecorder(VulkanDevice& inDevice, VulkanCommandBuffer& inCmdBuffer)
        : device(inDevice)
        , commandBuffer(inCmdBuffer)
    {
    }

    VulkanCommandRecorder::~VulkanCommandRecorder() = default;

    void VulkanCommandRecorder::ResourceBarrier(const Barrier& inBarrier)
    {
        if (inBarrier.type == ResourceType::buffer) {
            const auto& bufferBarrierInfo = inBarrier.buffer;
            const auto* nativeBuffer = static_cast<VulkanBuffer*>(bufferBarrierInfo.pointer);

            VkBufferMemoryBarrier bufferBarrier {};
            bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferBarrier.buffer = nativeBuffer->GetNative();
            bufferBarrier.size = nativeBuffer->GetCreateInfo().size;
            bufferBarrier.offset = 0;
            bufferBarrier.srcAccessMask = GetBufferMemoryBarrierAccessFlags(bufferBarrierInfo.before);
            bufferBarrier.dstAccessMask = GetBufferMemoryBarrierAccessFlags(bufferBarrierInfo.after);
            bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier(
                commandBuffer.GetNative(),
                GetBufferPipelineBarrierSrcStage(bufferBarrierInfo.before), GetBufferPipelineBarrierDstStage(bufferBarrierInfo.after),
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr,
                1, &bufferBarrier,
                0, nullptr);
        } else if (inBarrier.type == ResourceType::texture) {
            const auto& textureBarrierInfo = inBarrier.texture;

            const auto* nativeTexture = static_cast<VulkanTexture*>(textureBarrierInfo.pointer);
            VkImageMemoryBarrier imageBarrier {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = nativeTexture->GetNative();
            imageBarrier.oldLayout = GetTextureLayout(textureBarrierInfo.before);
            imageBarrier.srcAccessMask = GetTextureMemoryBarrierAccessFlags(textureBarrierInfo.before);
            imageBarrier.newLayout = GetTextureLayout(textureBarrierInfo.after);
            imageBarrier.dstAccessMask = GetTextureMemoryBarrierAccessFlags(textureBarrierInfo.after);
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.subresourceRange = nativeTexture->GetNativeSubResourceFullRange();

            vkCmdPipelineBarrier(
                commandBuffer.GetNative(),
                GetTexturePipelineBarrierSrcStage(textureBarrierInfo.before), GetTexturePipelineBarrierDstStage(textureBarrierInfo.after),
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr,
                0, nullptr,
                1, &imageBarrier);
        } else {
            Unimplement();
        }
    }

    void VulkanCommandRecorder::BeginMarker(const std::string& inLabel)
    {
#if BUILD_CONFIG_DEBUG
        VkDebugUtilsLabelEXT labelInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        labelInfo.pLabelName = inLabel.c_str();
        labelInfo.color[0] = labelInfo.color[1] = labelInfo.color[2] = labelInfo.color[3] = 1.0f;

        auto* pfn = device.GetGpu().GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkCmdBeginDebugUtilsLabelEXT>("vkCmdBeginDebugUtilsLabelEXT");
        pfn(commandBuffer.GetNative(), &labelInfo);
#endif
    }

    void VulkanCommandRecorder::EndMarker()
    {
#if BUILD_CONFIG_DEBUG
        auto* pfn = device.GetGpu().GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkCmdEndDebugUtilsLabelEXT>("vkCmdEndDebugUtilsLabelEXT");
        pfn(commandBuffer.GetNative());
#endif
    }

    Common::UniquePtr<CopyPassCommandRecorder> VulkanCommandRecorder::BeginCopyPass()
    {
        return Common::UniquePtr<CopyPassCommandRecorder>(new VulkanCopyPassCommandRecorder(device, *this, commandBuffer));
    }

    Common::UniquePtr<ComputePassCommandRecorder> VulkanCommandRecorder::BeginComputePass()
    {
        return Common::UniquePtr<ComputePassCommandRecorder>(new VulkanComputePassCommandRecorder(device, *this, commandBuffer));
    }

    Common::UniquePtr<RasterPassCommandRecorder> VulkanCommandRecorder::BeginRasterPass(const RasterPassBeginInfo& inBeginInfo)
    {
        return Common::UniquePtr<RasterPassCommandRecorder>(new VulkanRasterPassCommandRecorder(device, *this, commandBuffer, inBeginInfo));
    }

    void VulkanCommandRecorder::WriteTimestamp(QuerySet* inQuerySet, const uint32_t inQueryIndex)
    {
        const auto* querySet = static_cast<VulkanQuerySet*>(inQuerySet);
        vkCmdWriteTimestamp(commandBuffer.GetNative(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, querySet->GetNative(), inQueryIndex);
    }

    void VulkanCommandRecorder::ResetQuerySet(QuerySet* inQuerySet, const uint32_t inFirstQuery, const uint32_t inQueryCount)
    {
        const auto* querySet = static_cast<VulkanQuerySet*>(inQuerySet);
        vkCmdResetQueryPool(commandBuffer.GetNative(), querySet->GetNative(), inFirstQuery, inQueryCount);
    }

    void VulkanCommandRecorder::ResolveQuery(QuerySet* inQuerySet, const uint32_t inFirstQuery, const uint32_t inQueryCount, Buffer* inDstBuffer, const size_t inDstOffset)
    {
        const auto* querySet = static_cast<VulkanQuerySet*>(inQuerySet);
        const auto* dstBuffer = static_cast<VulkanBuffer*>(inDstBuffer);
        vkCmdCopyQueryPoolResults(
            commandBuffer.GetNative(), querySet->GetNative(), inFirstQuery, inQueryCount,
            dstBuffer->GetNative(), inDstOffset, sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }

    void VulkanCommandRecorder::End()
    {
        vkEndCommandBuffer(commandBuffer.GetNative());
    }

    VulkanCopyPassCommandRecorder::VulkanCopyPassCommandRecorder(VulkanDevice& inDevice, VulkanCommandRecorder& inCmdRecorder, VulkanCommandBuffer& inCmdBuffer)
        : device(inDevice)
        , commandRecorder(inCmdRecorder)
        , commandBuffer(inCmdBuffer)
    {
    }

    VulkanCopyPassCommandRecorder::~VulkanCopyPassCommandRecorder() = default;

    void VulkanCopyPassCommandRecorder::ResourceBarrier(const Barrier& inBarrier)
    {
        commandRecorder.ResourceBarrier(inBarrier);
    }

    void VulkanCopyPassCommandRecorder::BeginMarker(const std::string& inLabel)
    {
        commandRecorder.BeginMarker(inLabel);
    }

    void VulkanCopyPassCommandRecorder::EndMarker()
    {
        commandRecorder.EndMarker();
    }

    void VulkanCopyPassCommandRecorder::CopyBufferToBuffer(Buffer* src, Buffer* dst, const BufferCopyInfo& copyInfo)
    {
        const auto* srcBuffer = static_cast<VulkanBuffer*>(src);
        const auto* dstBuffer = static_cast<VulkanBuffer*>(dst);

        VkBufferCopy nativeBufferCopy {};
        nativeBufferCopy.srcOffset = copyInfo.srcOffset;
        nativeBufferCopy.dstOffset = copyInfo.dstOffset;
        nativeBufferCopy.size = copyInfo.copySize;

        vkCmdCopyBuffer(commandBuffer.GetNative(), srcBuffer->GetNative(), dstBuffer->GetNative(), 1, &nativeBufferCopy);
    }

    void VulkanCopyPassCommandRecorder::CopyBufferToTexture(Buffer* src, Texture* dst, const BufferTextureCopyInfo& copyInfo)
    {
        const auto* srcBuffer = static_cast<VulkanBuffer*>(src);
        const auto* dstTexture = static_cast<VulkanTexture*>(dst);

        RHI::Internal::ValidateBufferTextureCopy(*src, *dst, copyInfo);
        const VkBufferImageCopy nativeBufferImageCopy = GetNativeBufferImageCopy(*dst, copyInfo);
        vkCmdCopyBufferToImage(commandBuffer.GetNative(), srcBuffer->GetNative(), dstTexture->GetNative(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &nativeBufferImageCopy);
    }

    void VulkanCopyPassCommandRecorder::CopyTextureToBuffer(Texture* src, Buffer* dst, const BufferTextureCopyInfo& copyInfo)
    {
        const auto* srcTexture = static_cast<VulkanTexture*>(src);
        const auto* dstBuffer = static_cast<VulkanBuffer*>(dst);

        RHI::Internal::ValidateBufferTextureCopy(*dst, *src, copyInfo);
        const VkBufferImageCopy nativeBufferImageCopy = GetNativeBufferImageCopy(*src, copyInfo);
        vkCmdCopyImageToBuffer(commandBuffer.GetNative(), srcTexture->GetNative(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer->GetNative(), 1, &nativeBufferImageCopy);
    }

    void VulkanCopyPassCommandRecorder::CopyTextureToTexture(Texture* src, Texture* dst, const TextureCopyInfo& copyInfo)
    {
        const auto* srcTexture = static_cast<VulkanTexture*>(src);
        const auto* dstTexture = static_cast<VulkanTexture*>(dst);

        VkImageCopy nativeImageCopy {};
        nativeImageCopy.srcSubresource = GetNativeImageSubResourceLayers(copyInfo.srcSubResource);
        nativeImageCopy.srcOffset = { static_cast<int32_t>(copyInfo.srcOrigin.x), static_cast<int32_t>(copyInfo.srcOrigin.y), static_cast<int32_t>(copyInfo.srcOrigin.z) };
        nativeImageCopy.dstSubresource = GetNativeImageSubResourceLayers(copyInfo.dstSubResource);
        nativeImageCopy.dstOffset = { static_cast<int32_t>(copyInfo.dstOrigin.x), static_cast<int32_t>(copyInfo.dstOrigin.y), static_cast<int32_t>(copyInfo.dstOrigin.z) };
        nativeImageCopy.extent = { copyInfo.copyRegion.x, copyInfo.copyRegion.y, copyInfo.copyRegion.z };

        vkCmdCopyImage(commandBuffer.GetNative(), srcTexture->GetNative(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture->GetNative(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &nativeImageCopy);
    }

    void VulkanCopyPassCommandRecorder::EndPass()
    {
    }

    VulkanComputePassCommandRecorder::VulkanComputePassCommandRecorder(VulkanDevice& inDevice, VulkanCommandRecorder& inCmdRecorder, VulkanCommandBuffer& inCmdBuffer)
        : device(inDevice)
        , commandRecorder(inCmdRecorder)
        , commandBuffer(inCmdBuffer)
        , computePipeline(nullptr)
    {
    }

    VulkanComputePassCommandRecorder::~VulkanComputePassCommandRecorder() = default;

    void VulkanComputePassCommandRecorder::ResourceBarrier(const Barrier& inBarrier)
    {
        commandRecorder.ResourceBarrier(inBarrier);
    }

    void VulkanComputePassCommandRecorder::BeginMarker(const std::string& inLabel)
    {
        commandRecorder.BeginMarker(inLabel);
    }

    void VulkanComputePassCommandRecorder::EndMarker()
    {
        commandRecorder.EndMarker();
    }

    void VulkanComputePassCommandRecorder::SetPipeline(ComputePipeline* inPipeline)
    {
        computePipeline = static_cast<VulkanComputePipeline*>(inPipeline);
        Assert(computePipeline);

        vkCmdBindPipeline(commandBuffer.GetNative(), VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->GetNative());
    }

    void VulkanComputePassCommandRecorder::SetBindGroup(uint8_t inLayoutIndex, BindGroup* inBindGroup)
    {
        auto* vBindGroup = static_cast<VulkanBindGroup*>(inBindGroup);
        VkDescriptorSet descriptorSet = vBindGroup->GetNative();
        VkPipelineLayout layout = computePipeline->GetPipelineLayout()->GetNative();

        vkCmdBindDescriptorSets(commandBuffer.GetNative(), VK_PIPELINE_BIND_POINT_COMPUTE, layout, inLayoutIndex, 1, &descriptorSet, 0, nullptr);
    }

    void VulkanComputePassCommandRecorder::SetPipelineConstants(uint32_t inPipelineConstantIndex, const void* inData, uint32_t inSize)
    {
        const auto* pipelineLayout = computePipeline->GetPipelineLayout();
        const auto& range = pipelineLayout->GetPushConstantRange(inPipelineConstantIndex);
        vkCmdPushConstants(commandBuffer.GetNative(), pipelineLayout->GetNative(), range.stageFlags, range.offset, inSize, inData);
    }

    void VulkanComputePassCommandRecorder::Dispatch(size_t inGroupCountX, size_t inGroupCountY, size_t inGroupCountZ)
    {
        vkCmdDispatch(commandBuffer.GetNative(), inGroupCountX, inGroupCountY, inGroupCountZ);
    }

    void VulkanComputePassCommandRecorder::DispatchIndirect(Buffer* inIndirectBuffer, const size_t inOffset)
    {
        const auto* indirectBuffer = static_cast<VulkanBuffer*>(inIndirectBuffer);
        vkCmdDispatchIndirect(commandBuffer.GetNative(), indirectBuffer->GetNative(), inOffset);
    }

    void VulkanComputePassCommandRecorder::EndPass()
    {

    }

    VulkanRasterPassCommandRecorder::VulkanRasterPassCommandRecorder(VulkanDevice& inDevice, VulkanCommandRecorder& inCmdRecorder, VulkanCommandBuffer& inCmdBuffer, const RasterPassBeginInfo& inBeginInfo)
        : device(inDevice)
        , commandRecorder(inCmdRecorder)
        , commandBuffer(inCmdBuffer)
        , rasterPipeline(nullptr)
        , activeOcclusionQuerySet(nullptr)
        , activeOcclusionQueryIndex(0)
    {
        std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos(inBeginInfo.colorAttachments.size());
        const VulkanTextureView* referenceAttachmentView = nullptr;
        for (size_t i = 0; i < inBeginInfo.colorAttachments.size(); i++)
        {
            const auto* colorTextureView = static_cast<VulkanTextureView*>(inBeginInfo.colorAttachments[i].view);
            if (referenceAttachmentView == nullptr) {
                referenceAttachmentView = colorTextureView;
            }
            colorAttachmentInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorAttachmentInfos[i].imageView = colorTextureView->GetNative();
            colorAttachmentInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentInfos[i].loadOp = EnumCast<LoadOp, VkAttachmentLoadOp>(inBeginInfo.colorAttachments[i].loadOp);
            colorAttachmentInfos[i].storeOp = EnumCast<StoreOp, VkAttachmentStoreOp>(inBeginInfo.colorAttachments[i].storeOp);
            colorAttachmentInfos[i].clearValue.color = {
                inBeginInfo.colorAttachments[i].clearValue.r,
                inBeginInfo.colorAttachments[i].clearValue.g,
                inBeginInfo.colorAttachments[i].clearValue.b,
                inBeginInfo.colorAttachments[i].clearValue.a
                    };
        }

        VkRenderingInfoKHR renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
        renderingInfo.pColorAttachments = colorAttachmentInfos.empty() ? nullptr : colorAttachmentInfos.data();
        renderingInfo.viewMask = 0;

        VkRenderingAttachmentInfo depthAttachmentInfo = {};
        VkRenderingAttachmentInfo stencilAttachmentInfo = {};

        if (inBeginInfo.depthStencilAttachment.has_value())
        {
            const auto* depthStencilTextureView = static_cast<VulkanTextureView*>(inBeginInfo.depthStencilAttachment->view);
            if (referenceAttachmentView == nullptr) {
                referenceAttachmentView = depthStencilTextureView;
            }

            const auto& attachment = inBeginInfo.depthStencilAttachment.value();
            const auto aspect = depthStencilTextureView->GetCreateInfo().aspect;
            const bool hasDepth = aspect == TextureAspect::depth || aspect == TextureAspect::depthStencil;
            const bool hasStencil = aspect == TextureAspect::stencil || aspect == TextureAspect::depthStencil;
            const auto imageLayout = GetTextureLayout(GetDepthStencilTextureState(aspect, attachment.depthReadOnly, attachment.stencilReadOnly));

            AssertWithReason(!hasDepth || !attachment.depthReadOnly || attachment.depthLoadOp != LoadOp::clear, "read-only depth attachments cannot be cleared");
            AssertWithReason(!hasStencil || !attachment.stencilReadOnly || attachment.stencilLoadOp != LoadOp::clear, "read-only stencil attachments cannot be cleared");

            if (hasDepth) {
                depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAttachmentInfo.imageView = depthStencilTextureView->GetNative();
                depthAttachmentInfo.imageLayout = imageLayout;
                depthAttachmentInfo.loadOp = EnumCast<LoadOp, VkAttachmentLoadOp>(attachment.depthLoadOp);
                depthAttachmentInfo.storeOp = EnumCast<StoreOp, VkAttachmentStoreOp>(attachment.depthStoreOp);
                depthAttachmentInfo.clearValue.depthStencil = { attachment.depthClearValue, attachment.stencilClearValue };
                renderingInfo.pDepthAttachment = &depthAttachmentInfo;
            }

            if (hasStencil) {
                stencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                stencilAttachmentInfo.imageView = depthStencilTextureView->GetNative();
                stencilAttachmentInfo.imageLayout = imageLayout;
                stencilAttachmentInfo.loadOp = EnumCast<LoadOp, VkAttachmentLoadOp>(attachment.stencilLoadOp);
                stencilAttachmentInfo.storeOp = EnumCast<StoreOp, VkAttachmentStoreOp>(attachment.stencilStoreOp);
                stencilAttachmentInfo.clearValue.depthStencil = { attachment.depthClearValue, attachment.stencilClearValue };
                renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
            }
        }

        AssertWithReason(referenceAttachmentView != nullptr, "raster passes must have at least one attachment");
        const auto& textureCreateInfo = referenceAttachmentView->GetTexture().GetCreateInfo();
        const auto& textureViewCreateInfo = referenceAttachmentView->GetCreateInfo();
        renderingInfo.layerCount = textureViewCreateInfo.arrayLayerNum;
        renderingInfo.renderArea = {{0, 0}, {std::max(textureCreateInfo.width >> textureViewCreateInfo.baseMipLevel, 1u), std::max(textureCreateInfo.height >> textureViewCreateInfo.baseMipLevel, 1u)}};

        auto* pfn = device.GetGpu().GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkCmdBeginRenderingKHR>("vkCmdBeginRenderingKHR");
        pfn(commandBuffer.GetNative(), &renderingInfo);
    }

    VulkanRasterPassCommandRecorder::~VulkanRasterPassCommandRecorder() = default;

    void VulkanRasterPassCommandRecorder::ResourceBarrier(const Barrier& inBarrier)
    {
        commandRecorder.ResourceBarrier(inBarrier);
    }

    void VulkanRasterPassCommandRecorder::BeginMarker(const std::string& inLabel)
    {
        commandRecorder.BeginMarker(inLabel);
    }

    void VulkanRasterPassCommandRecorder::EndMarker()
    {
        commandRecorder.EndMarker();
    }

    void VulkanRasterPassCommandRecorder::SetPipeline(RasterPipeline* inPipeline)
    {
        rasterPipeline = static_cast<VulkanRasterPipeline*>(inPipeline);
        Assert(rasterPipeline);

       vkCmdBindPipeline(commandBuffer.GetNative(), VK_PIPELINE_BIND_POINT_GRAPHICS, rasterPipeline->GetNative());
    }

    void VulkanRasterPassCommandRecorder::SetBindGroup(uint8_t inLayoutIndex, BindGroup* inBindGroup)
    {
        const auto* vBindGroup = static_cast<VulkanBindGroup*>(inBindGroup);
        const VkDescriptorSet descriptorSet = vBindGroup->GetNative();
        const VkPipelineLayout layout = rasterPipeline->GetPipelineLayout()->GetNative();

        vkCmdBindDescriptorSets(commandBuffer.GetNative(), VK_PIPELINE_BIND_POINT_GRAPHICS, layout, inLayoutIndex, 1, &descriptorSet, 0, nullptr);
    }

    void VulkanRasterPassCommandRecorder::SetPipelineConstants(uint32_t inPipelineConstantIndex, const void* inData, uint32_t inSize)
    {
        const auto* pipelineLayout = rasterPipeline->GetPipelineLayout();
        const auto& range = pipelineLayout->GetPushConstantRange(inPipelineConstantIndex);
        vkCmdPushConstants(commandBuffer.GetNative(), pipelineLayout->GetNative(), range.stageFlags, range.offset, inSize, inData);
    }

    void VulkanRasterPassCommandRecorder::SetIndexBuffer(BufferView *inBufferView)
    {
        const auto* bufferView = static_cast<VulkanBufferView*>(inBufferView);
        const auto& createInfo = bufferView->GetCreateInfo();
        Assert(createInfo.type == BufferViewType::index);

        const VkBuffer indexBuffer = bufferView->GetBuffer().GetNative();
        const auto indexFormat = std::get<IndexBufferViewInfo>(createInfo.extend).format;
        const auto vkFormat = EnumCast<IndexFormat, VkIndexType>(indexFormat);

        vkCmdBindIndexBuffer(commandBuffer.GetNative(), indexBuffer, createInfo.offsetInBytes, vkFormat);
    }

    void VulkanRasterPassCommandRecorder::SetVertexBuffer(size_t inSlot, BufferView *inBufferView)
    {
        const auto* bufferView = static_cast<VulkanBufferView*>(inBufferView);
        const auto& createInfo = bufferView->GetCreateInfo();
        Assert(createInfo.type == BufferViewType::vertex);

        const VkBuffer vertexBuffer = bufferView->GetBuffer().GetNative();
        const VkDeviceSize offset[] = { createInfo.offsetInBytes };
        vkCmdBindVertexBuffers(commandBuffer.GetNative(), inSlot, 1, &vertexBuffer, offset);
    }

    void VulkanRasterPassCommandRecorder::Draw(const size_t inVertexCount, const size_t inInstanceCount, const size_t inFirstVertex, const size_t inFirstInstance)
    {
        vkCmdDraw(commandBuffer.GetNative(), inVertexCount, inInstanceCount, inFirstVertex, inFirstInstance);
    }

    void VulkanRasterPassCommandRecorder::DrawIndexed(const size_t inIndexCount, const size_t inInstanceCount, const size_t inFirstIndex, const size_t inBaseVertex, const size_t inFirstInstance)
    {
        vkCmdDrawIndexed(commandBuffer.GetNative(), inIndexCount, inInstanceCount, inFirstIndex, inBaseVertex, inFirstInstance);
    }

    void VulkanRasterPassCommandRecorder::SetViewport(const float inX, const float inY, const float inWidth, const float inHeight, const float inMinDepth, const float inMaxDepth)
    {
        VkViewport viewport{};
        viewport.x = inX;
        viewport.y = inY;
        viewport.width = inWidth;
        viewport.height = inHeight;
        viewport.minDepth = inMinDepth;
        viewport.maxDepth = inMaxDepth;
        vkCmdSetViewport(commandBuffer.GetNative(), 0, 1, &viewport);
    }

    void VulkanRasterPassCommandRecorder::SetScissor(const uint32_t inLeft, const uint32_t inTop, const uint32_t inRight, const uint32_t inBottom)
    {
        VkRect2D rect;
        rect.offset = {static_cast<int32_t>(inLeft), static_cast<int32_t>(inTop) };
        rect.extent = {inRight - inLeft, inBottom - inTop };
        vkCmdSetScissor(commandBuffer.GetNative(), 0, 1, &rect);
    }

    void VulkanRasterPassCommandRecorder::SetPrimitiveTopology(PrimitiveTopology inPrimitiveTopology)
    {
#if PLATFORM_MACOS
        // MoltenVK not support use vkCmdSetPrimitiveTopology() directly current
        auto* pfn = device.GetGpu().GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkCmdSetPrimitiveTopologyEXT>("vkCmdSetPrimitiveTopologyEXT");
        pfn(commandBuffer.GetNative(), EnumCast<PrimitiveTopology, VkPrimitiveTopology>(inPrimitiveTopology));
#else
        vkCmdSetPrimitiveTopology(commandBuffer.GetNative(), EnumCast<PrimitiveTopology, VkPrimitiveTopology>(inPrimitiveTopology));
#endif
    }

    void VulkanRasterPassCommandRecorder::SetBlendConstant(const float *inConstants)
    {
        vkCmdSetBlendConstants(commandBuffer.GetNative(), inConstants);
    }

    void VulkanRasterPassCommandRecorder::SetStencilReference(const uint32_t inReference)
    {
        // TODO stencil face;
        vkCmdSetStencilReference(commandBuffer.GetNative(), VK_STENCIL_FACE_FRONT_AND_BACK, inReference);
    }

    void VulkanRasterPassCommandRecorder::DrawIndirect(Buffer* inIndirectBuffer, const size_t inOffset)
    {
        MultiDrawIndirect(inIndirectBuffer, inOffset, 1);
    }

    void VulkanRasterPassCommandRecorder::DrawIndexedIndirect(Buffer* inIndirectBuffer, const size_t inOffset)
    {
        MultiDrawIndexedIndirect(inIndirectBuffer, inOffset, 1);
    }

    void VulkanRasterPassCommandRecorder::MultiDrawIndirect(Buffer* inIndirectBuffer, const size_t inOffset, const size_t inDrawCount)
    {
        const auto* indirectBuffer = static_cast<VulkanBuffer*>(inIndirectBuffer);
        vkCmdDrawIndirect(commandBuffer.GetNative(), indirectBuffer->GetNative(), inOffset, inDrawCount, sizeof(DrawIndirectArguments));
    }

    void VulkanRasterPassCommandRecorder::MultiDrawIndexedIndirect(Buffer* inIndirectBuffer, const size_t inOffset, const size_t inDrawCount)
    {
        const auto* indirectBuffer = static_cast<VulkanBuffer*>(inIndirectBuffer);
        vkCmdDrawIndexedIndirect(commandBuffer.GetNative(), indirectBuffer->GetNative(), inOffset, inDrawCount, sizeof(DrawIndexedIndirectArguments));
    }

    void VulkanRasterPassCommandRecorder::BeginOcclusionQuery(QuerySet* inQuerySet, const uint32_t inQueryIndex)
    {
        activeOcclusionQuerySet = static_cast<VulkanQuerySet*>(inQuerySet);
        activeOcclusionQueryIndex = inQueryIndex;
        const VkQueryControlFlags queryFlags = device.GetEnabledFeatures().occlusionQueryPrecise == VK_TRUE ? VK_QUERY_CONTROL_PRECISE_BIT : 0;
        vkCmdBeginQuery(commandBuffer.GetNative(), activeOcclusionQuerySet->GetNative(), inQueryIndex, queryFlags);
    }

    void VulkanRasterPassCommandRecorder::EndOcclusionQuery()
    {
        Assert(activeOcclusionQuerySet != nullptr);
        vkCmdEndQuery(commandBuffer.GetNative(), activeOcclusionQuerySet->GetNative(), activeOcclusionQueryIndex);
    }

    void VulkanRasterPassCommandRecorder::EndPass()
    {
        auto* pfn = device.GetGpu().GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkCmdEndRenderingKHR>("vkCmdEndRenderingKHR");
        pfn(commandBuffer.GetNative());
    }
}

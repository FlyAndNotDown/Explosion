//
// Created by johnk on 21/2/2022.
//

#include <algorithm>
#include <limits>

#include <RHI/CommandRecorder.h>
#include <RHI/Buffer.h>
#include <RHI/Texture.h>

namespace RHI::Internal {
    void ValidateBufferTextureCopy(const Buffer& buffer, const Texture& texture, const BufferTextureCopyInfo& copyInfo)
    {
        const auto& textureCreateInfo = texture.GetCreateInfo();
        Assert(copyInfo.textureSubResource.mipLevel < textureCreateInfo.mipLevels);

        const auto arraySize = textureCreateInfo.type == TextureType::t3D ? 1u : textureCreateInfo.depthOrArraySize;
        Assert(copyInfo.textureSubResource.arrayLayer < arraySize);

        const auto mipLevel = copyInfo.textureSubResource.mipLevel;
        const auto baseDepth = textureCreateInfo.type == TextureType::t3D ? textureCreateInfo.depthOrArraySize : 1u;
        const Common::UVec3 subResourceExtent = {
            std::max(textureCreateInfo.width >> mipLevel, 1u),
            std::max(textureCreateInfo.height >> mipLevel, 1u),
            std::max(baseDepth >> mipLevel, 1u)
        };

        Assert(copyInfo.copyRegion.x > 0 && copyInfo.copyRegion.y > 0 && copyInfo.copyRegion.z > 0);
        Assert(copyInfo.textureOrigin.x <= subResourceExtent.x && copyInfo.copyRegion.x <= subResourceExtent.x - copyInfo.textureOrigin.x);
        Assert(copyInfo.textureOrigin.y <= subResourceExtent.y && copyInfo.copyRegion.y <= subResourceExtent.y - copyInfo.textureOrigin.y);
        Assert(copyInfo.textureOrigin.z <= subResourceExtent.z && copyInfo.copyRegion.z <= subResourceExtent.z - copyInfo.textureOrigin.z);

        const auto bytesPerPixel = GetBytesPerPixel(textureCreateInfo.format);
        Assert(copyInfo.copyRegion.x <= std::numeric_limits<size_t>::max() / bytesPerPixel);
        const auto packedRowPitch = bytesPerPixel * copyInfo.copyRegion.x;
        Assert(copyInfo.bufferRowPitch >= packedRowPitch && copyInfo.bufferRowPitch % bytesPerPixel == 0);
        Assert(copyInfo.copyRegion.y <= std::numeric_limits<size_t>::max() / copyInfo.bufferRowPitch);
        Assert(copyInfo.bufferSlicePitch >= copyInfo.bufferRowPitch * copyInfo.copyRegion.y);
        Assert(copyInfo.bufferSlicePitch % copyInfo.bufferRowPitch == 0);
        Assert(copyInfo.bufferRowPitch <= std::numeric_limits<uint32_t>::max());
        Assert(copyInfo.bufferSlicePitch / copyInfo.bufferRowPitch <= std::numeric_limits<uint32_t>::max());

        const auto bufferSize = static_cast<size_t>(buffer.GetCreateInfo().size);
        Assert(copyInfo.bufferOffset <= bufferSize);
        Assert(copyInfo.bufferSlicePitch <= (bufferSize - copyInfo.bufferOffset) / copyInfo.copyRegion.z);
    }
}

namespace RHI {
    TextureSubResourceInfo::TextureSubResourceInfo(
        const uint8_t inMipLevel,
        const uint8_t inArrayLayer,
        const TextureAspect inAspect)
        : mipLevel(inMipLevel)
        , arrayLayer(inArrayLayer)
        , aspect(inAspect)
    {
    }

    TextureSubResourceInfo& TextureSubResourceInfo::SetMipLevel(const uint8_t inMipLevel)
    {
        mipLevel = inMipLevel;
        return *this;
    }

    TextureSubResourceInfo& TextureSubResourceInfo::SetArrayLayer(const uint8_t inArrayLayer)
    {
        arrayLayer = inArrayLayer;
        return *this;
    }

    TextureSubResourceInfo& TextureSubResourceInfo::SetAspect(const TextureAspect inAspect)
    {
        aspect = inAspect;
        return *this;
    }

    BufferCopyInfo::BufferCopyInfo(const size_t inSrcOffset, const size_t inDstOffset, const size_t inCopySize)
        : srcOffset(inSrcOffset)
        , dstOffset(inDstOffset)
        , copySize(inCopySize)
    {
    }

    BufferCopyInfo& BufferCopyInfo::SetSrcOffset(const size_t inSrcOffset)
    {
        srcOffset = inSrcOffset;
        return *this;
    }

    BufferCopyInfo& BufferCopyInfo::SetDstOffset(const size_t inDstOffset)
    {
        dstOffset = inDstOffset;
        return *this;
    }

    BufferCopyInfo& BufferCopyInfo::SetCopySize(const size_t inCopySize)
    {
        copySize = inCopySize;
        return *this;
    }

    TextureCopyInfo::TextureCopyInfo(const TextureSubResourceInfo& inSrcSubResource, const Common::UVec3& inSrcOrigin, const TextureSubResourceInfo& inDstSubResource, const Common::UVec3& inDstOrigin, const Common::UVec3& inCopyRegion)
        : srcSubResource(inSrcSubResource)
        , srcOrigin(inSrcOrigin)
        , dstSubResource(inDstSubResource)
        , dstOrigin(inDstOrigin)
        , copyRegion(inCopyRegion)
    {
    }

    TextureCopyInfo& TextureCopyInfo::SetSrcSubResource(const TextureSubResourceInfo& inSrcSubResource)
    {
        srcSubResource = inSrcSubResource;
        return *this;
    }

    TextureCopyInfo& TextureCopyInfo::SetSrcOrigin(const Common::UVec3& inSrcOrigin)
    {
        srcOrigin = inSrcOrigin;
        return *this;
    }

    TextureCopyInfo& TextureCopyInfo::SetDstSubResource(const TextureSubResourceInfo& inDstSubResource)
    {
        dstSubResource = inDstSubResource;
        return *this;
    }

    TextureCopyInfo& TextureCopyInfo::SetDstOrigin(const Common::UVec3& inDstOrigin)
    {
        dstOrigin = inDstOrigin;
        return *this;
    }

    TextureCopyInfo& TextureCopyInfo::SetCopyRegion(const Common::UVec3& inCopyRegion)
    {
        copyRegion = inCopyRegion;
        return *this;
    }

    BufferTextureCopyInfo::BufferTextureCopyInfo(const size_t inBufferOffset, const TextureSubResourceInfo& inTextureSubResource, const Common::UVec3& inTextureOrigin, const Common::UVec3& inCopyRegion, const size_t inBufferRowPitch, const size_t inBufferSlicePitch)
        : bufferOffset(inBufferOffset)
        , bufferRowPitch(inBufferRowPitch)
        , bufferSlicePitch(inBufferSlicePitch)
        , textureSubResource(inTextureSubResource)
        , textureOrigin(inTextureOrigin)
        , copyRegion(inCopyRegion)
    {
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetBufferOffset(const size_t inBufferOffset)
    {
        bufferOffset = inBufferOffset;
        return *this;
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetBufferRowPitch(const size_t inBufferRowPitch)
    {
        bufferRowPitch = inBufferRowPitch;
        return *this;
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetBufferSlicePitch(const size_t inBufferSlicePitch)
    {
        bufferSlicePitch = inBufferSlicePitch;
        return *this;
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetTextureSubResource(const TextureSubResourceInfo& inTextureSubResource)
    {
        textureSubResource = inTextureSubResource;
        return *this;
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetTextureOrigin(const Common::UVec3& inTextureOrigin)
    {
        textureOrigin = inTextureOrigin;
        return *this;
    }

    BufferTextureCopyInfo& BufferTextureCopyInfo::SetCopyRegion(const Common::UVec3& inCopyRegion)
    {
        copyRegion = inCopyRegion;
        return *this;
    }

    ColorAttachment::ColorAttachment(
        TextureView* inView,
        const LoadOp inLoadOp,
        const StoreOp inStoreOp,
        const Common::LinearColor& inClearValue,
        TextureView* inResolveView)
        : ColorAttachmentBase(inLoadOp, inStoreOp, inClearValue)
        , view(inView)
        , resolveView(inResolveView)
    {
    }

    ColorAttachment& ColorAttachment::SetView(TextureView* inView)
    {
        view = inView;
        return *this;
    }

    ColorAttachment& ColorAttachment::SetResolveView(TextureView* inResolveView)
    {
        resolveView = inResolveView;
        return *this;
    }

    DepthStencilAttachment::DepthStencilAttachment(
        TextureView* inView,
        const bool inDepthReadOnly,
        const LoadOp inDepthLoadOp,
        const StoreOp inDepthStoreOp,
        const float inDepthClearValue,
        const bool inStencilReadOnly,
        const LoadOp inStencilLoadOp,
        const StoreOp inStencilStoreOp,
        const uint32_t inStencilClearValue)
        : DepthStencilAttachmentBase(
            inDepthReadOnly, inDepthLoadOp, inDepthStoreOp, inDepthClearValue,
            inStencilReadOnly, inStencilLoadOp, inStencilStoreOp, inStencilClearValue)
        , view(inView)
    {
    }

    DepthStencilAttachment& DepthStencilAttachment::SetView(TextureView* inView)
    {
        view = inView;
        return *this;
    }

    RasterPassBeginInfo::RasterPassBeginInfo() = default;

    RasterPassBeginInfo& RasterPassBeginInfo::SetDepthStencilAttachment(const DepthStencilAttachment& inDepthStencilAttachment)
    {
        depthStencilAttachment = inDepthStencilAttachment;
        return *this;
    }

    RasterPassBeginInfo& RasterPassBeginInfo::AddColorAttachment(const ColorAttachment& inColorAttachment)
    {
        colorAttachments.emplace_back(inColorAttachment);
        return *this;
    }

    CommandRecorder::CommandRecorder() = default;

    CommandRecorder::~CommandRecorder() = default;

    CommonCommandRecorder::~CommonCommandRecorder() = default;

    CopyPassCommandRecorder::CopyPassCommandRecorder() = default;

    CopyPassCommandRecorder::~CopyPassCommandRecorder() = default;

    ComputePassCommandRecorder::ComputePassCommandRecorder() = default;

    ComputePassCommandRecorder::~ComputePassCommandRecorder() = default;

    RasterPassCommandRecorder::RasterPassCommandRecorder() = default;

    RasterPassCommandRecorder::~RasterPassCommandRecorder() = default;

    ScopedMarker::ScopedMarker(CommonCommandRecorder& inRecorder, const std::string& inLabel)
        : recorder(inRecorder)
    {
        recorder.BeginMarker(inLabel);
    }

    ScopedMarker::~ScopedMarker()
    {
        recorder.EndMarker();
    }
}

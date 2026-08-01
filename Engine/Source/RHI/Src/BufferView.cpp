//
// Created by johnk on 20/3/2022.
//

#include <RHI/BufferView.h>

namespace RHI {
    VertexBufferViewInfo::VertexBufferViewInfo(uint32_t inStride)
        : stride(inStride)
    {
    }

    IndexBufferViewInfo::IndexBufferViewInfo(IndexFormat inFormat)
        : format(inFormat)
    {
    }

    StorageBufferViewInfo::StorageBufferViewInfo(uint32_t inStride)
        : stride(inStride)
    {
    }

    BufferViewCreateInfo::BufferViewCreateInfo(
        const BufferViewType inType,
        const uint32_t inSizeInBytes,
        const uint32_t inOffsetInBytes,
        const std::variant<VertexBufferViewInfo, IndexBufferViewInfo, StorageBufferViewInfo>& inExtent)
        : type(inType)
        , sizeInBytes(inSizeInBytes)
        , offsetInBytes(inOffsetInBytes)
        , extend(inExtent)
    {
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetType(const BufferViewType inType)
    {
        type = inType;
        return *this;
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetOffsetInBytes(const uint32_t inOffsetInBytes)
    {
        offsetInBytes = inOffsetInBytes;
        return *this;
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetSizeInBytes(const uint32_t inSizeInBytes)
    {
        sizeInBytes = inSizeInBytes;
        return *this;
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetExtendVertex(const uint32_t inStride)
    {
        extend = VertexBufferViewInfo { inStride };
        return *this;
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetExtendIndex(const IndexFormat inFormat)
    {
        extend = IndexBufferViewInfo { inFormat };
        return *this;
    }

    BufferViewCreateInfo& BufferViewCreateInfo::SetExtendStorage(uint32_t inStride)
    {
        extend = StorageBufferViewInfo {inStride};
        return *this;
    }

    BufferView::BufferView(const BufferViewCreateInfo& inCreateInfo)
        : createInfo(inCreateInfo)
    {
    }

    BufferView::~BufferView() = default;

    const BufferViewCreateInfo& BufferView::GetCreateInfo() const
    {
        return createInfo;
    }
}

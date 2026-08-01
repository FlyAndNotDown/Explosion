//
// Created by johnk on 20/3/2022.
//

#pragma once

#include <cstddef>
#include <variant>

#include <Common/Utility.h>
#include <RHI/Common.h>

namespace RHI {
    struct VertexBufferViewInfo {
        uint32_t stride;

        explicit VertexBufferViewInfo(uint32_t inStride = 0);
    };

    struct IndexBufferViewInfo {
        IndexFormat format;

        explicit IndexBufferViewInfo(IndexFormat inFormat = IndexFormat::max);
    };

    struct StorageBufferViewInfo {
        uint32_t stride;

        explicit StorageBufferViewInfo(uint32_t inStride = 0);
    };

    struct BufferViewCreateInfo {
        BufferViewType type;
        uint32_t sizeInBytes;
        uint32_t offsetInBytes;
        std::variant<VertexBufferViewInfo, IndexBufferViewInfo, StorageBufferViewInfo> extend;

        explicit BufferViewCreateInfo(
            BufferViewType inType = BufferViewType::max,
            uint32_t inSizeInBytes = 0,
            uint32_t inOffsetInBytes = 0,
            const std::variant<VertexBufferViewInfo, IndexBufferViewInfo, StorageBufferViewInfo>& inExtent = {});

        BufferViewCreateInfo& SetType(BufferViewType inType);
        BufferViewCreateInfo& SetOffsetInBytes(uint32_t inOffsetInBytes);
        BufferViewCreateInfo& SetSizeInBytes(uint32_t inSizeInBytes);
        BufferViewCreateInfo& SetExtendVertex(uint32_t inStride);
        BufferViewCreateInfo& SetExtendIndex(IndexFormat inFormat);
        BufferViewCreateInfo& SetExtendStorage(uint32_t inStride);
    };

    class BufferView {
    public:
        NonCopyable(BufferView)
        virtual ~BufferView();

        const BufferViewCreateInfo& GetCreateInfo() const;

    protected:
        explicit BufferView(const BufferViewCreateInfo& inCreateInfo);

        BufferViewCreateInfo createInfo;
    };
}

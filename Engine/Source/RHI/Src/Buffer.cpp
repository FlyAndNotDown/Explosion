//
// Created by johnk on 2022/1/23.
//

#include <RHI/Buffer.h>
#include <RHI/BufferView.h>

namespace RHI::Internal {
    static BufferUsageBits GetRequiredBufferUsage(const BufferViewType type)
    {
        switch (type) {
        case BufferViewType::vertex:
            return BufferUsageBits::vertex;
        case BufferViewType::index:
            return BufferUsageBits::index;
        case BufferViewType::uniformBinding:
            return BufferUsageBits::uniform;
        case BufferViewType::storageBinding:
            return BufferUsageBits::storage;
        case BufferViewType::rwStorageBinding:
            return BufferUsageBits::rwStorage;
        default:
            Unimplement();
            return BufferUsageBits::max;
        }
    }

    static uint32_t GetIndexElementSize(const IndexFormat format)
    {
        switch (format) {
        case IndexFormat::uint16:
            return sizeof(uint16_t);
        case IndexFormat::uint32:
            return sizeof(uint32_t);
        default:
            Unimplement();
            return 0;
        }
    }

    static void ValidateBufferViewCreateInfo(const BufferCreateInfo& bufferCreateInfo, const BufferViewCreateInfo& viewCreateInfo)
    {
        Assert(viewCreateInfo.sizeInBytes > 0);
        Assert(viewCreateInfo.offsetInBytes <= bufferCreateInfo.size);
        Assert(viewCreateInfo.sizeInBytes <= bufferCreateInfo.size - viewCreateInfo.offsetInBytes);
        Assert((bufferCreateInfo.usages & GetRequiredBufferUsage(viewCreateInfo.type)) != 0);

        if (viewCreateInfo.type == BufferViewType::vertex) {
            Assert(std::holds_alternative<VertexBufferViewInfo>(viewCreateInfo.extend));
            Assert(std::get<VertexBufferViewInfo>(viewCreateInfo.extend).stride > 0);
        } else if (viewCreateInfo.type == BufferViewType::index) {
            Assert(std::holds_alternative<IndexBufferViewInfo>(viewCreateInfo.extend));
            const uint32_t elementSize = GetIndexElementSize(std::get<IndexBufferViewInfo>(viewCreateInfo.extend).format);
            Assert(viewCreateInfo.offsetInBytes % elementSize == 0);
            Assert(viewCreateInfo.sizeInBytes % elementSize == 0);
        } else if (viewCreateInfo.type == BufferViewType::storageBinding || viewCreateInfo.type == BufferViewType::rwStorageBinding) {
            Assert(std::holds_alternative<StorageBufferViewInfo>(viewCreateInfo.extend));
            const uint32_t stride = std::get<StorageBufferViewInfo>(viewCreateInfo.extend).stride;
            Assert(stride > 0);
            Assert(viewCreateInfo.offsetInBytes % stride == 0);
            Assert(viewCreateInfo.sizeInBytes % stride == 0);
        }
    }
}

namespace RHI {
    BufferCreateInfo::BufferCreateInfo() = default;

    BufferCreateInfo::BufferCreateInfo(const uint32_t inSize, const BufferUsageFlags inUsages, const BufferState inInitialState, std::string inDebugName)
        : size(inSize)
        , usages(inUsages)
        , initialState(inInitialState)
        , debugName(std::move(inDebugName))
    {
    }

    BufferCreateInfo& BufferCreateInfo::SetSize(const uint32_t inSize)
    {
        size = inSize;
        return *this;
    }
    BufferCreateInfo& BufferCreateInfo::SetUsages(const BufferUsageFlags inUsages)
    {
        usages = inUsages;
        return *this;
    }
    BufferCreateInfo& BufferCreateInfo::SetInitialState(const BufferState inState)
    {
        initialState = inState;
        return *this;
    }

    BufferCreateInfo& BufferCreateInfo::SetDebugName(std::string inDebugName)
    {
        debugName = std::move(inDebugName);
        return *this;
    }

    bool BufferCreateInfo::operator==(const BufferCreateInfo& rhs) const
    {
        return size == rhs.size
            && usages == rhs.usages
            && initialState == rhs.initialState;
    }

    Buffer::Buffer(const BufferCreateInfo& inCreateInfo)
        : createInfo(inCreateInfo)
    {
    }

    Buffer::~Buffer() = default;

    const BufferCreateInfo& Buffer::GetCreateInfo() const
    {
        return createInfo;
    }

    Common::UniquePtr<BufferView> Buffer::CreateBufferView(const BufferViewCreateInfo& inCreateInfo)
    {
        Internal::ValidateBufferViewCreateInfo(createInfo, inCreateInfo);
        return CreateBufferViewInternal(inCreateInfo);
    }
}

//
// Created by johnk on 2023/3/21.
//

#include <RHI/Dummy/Buffer.h>
#include <RHI/Dummy/BufferView.h>
#include <Common/Debug.h>

namespace RHI::Dummy {
    DummyBuffer::DummyBuffer(const BufferCreateInfo& createInfo)
        : Buffer(createInfo)
        , dummyData(createInfo.size)
    {
    }

    DummyBuffer::~DummyBuffer() = default;

    void* DummyBuffer::Map(MapMode mapMode, size_t offset, size_t length)
    {
        Assert(offset <= dummyData.size());
        Assert(length <= dummyData.size() - offset);
        return dummyData.data() + offset;
    }

    void DummyBuffer::Unmap()
    {
    }

    Common::UniquePtr<BufferView> DummyBuffer::CreateBufferView(const BufferViewCreateInfo& createInfo)
    {
        return Common::UniquePtr<BufferView>(new DummyBufferView(createInfo));
    }
}

//
// Created by johnk on 2026/7/5.
//

#include <Render/MeshRenderData.h>

namespace Render::Internal {
    static Common::UniquePtr<RHI::Buffer> CreateUploadedBuffer(RHI::Device& inDevice, const void* inData, size_t inSize, RHI::BufferUsageBits inUsage, const std::string& inDebugName)
    {
        const RHI::BufferCreateInfo createInfo = RHI::BufferCreateInfo()
            .SetSize(inSize)
            .SetUsages(inUsage | RHI::BufferUsageBits::mapWrite | RHI::BufferUsageBits::copySrc)
            .SetInitialState(RHI::BufferState::staging)
            .SetDebugName(inDebugName);

        Common::UniquePtr<RHI::Buffer> result = inDevice.CreateBuffer(createInfo);
        Assert(result.Valid());
        auto* data = result->Map(RHI::MapMode::write, 0, inSize);
        memcpy(data, inData, inSize);
        result->Unmap();
        return result;
    }
}

namespace Render {
    MeshRenderData::MeshRenderData(RHI::Device& inDevice, const std::vector<Vertex>& inVertices, const std::vector<uint32_t>& inIndices)
        : vertexBuffer(Internal::CreateUploadedBuffer(inDevice, inVertices.data(), inVertices.size() * sizeof(Vertex), RHI::BufferUsageBits::vertex, "meshVertexBuffer"))
        , indexBuffer(Internal::CreateUploadedBuffer(inDevice, inIndices.data(), inIndices.size() * sizeof(uint32_t), RHI::BufferUsageBits::index, "meshIndexBuffer"))
        , indexCount(static_cast<uint32_t>(inIndices.size()))
    {
    }

    MeshRenderData::~MeshRenderData() = default;

    RHI::Buffer* MeshRenderData::GetVertexBuffer() const
    {
        return vertexBuffer.Get();
    }

    RHI::Buffer* MeshRenderData::GetIndexBuffer() const
    {
        return indexBuffer.Get();
    }

    uint32_t MeshRenderData::GetIndexCount() const
    {
        return indexCount;
    }
}

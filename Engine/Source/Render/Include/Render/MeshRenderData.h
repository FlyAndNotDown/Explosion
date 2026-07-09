//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <vector>

#include <Common/Math/Vector.h>
#include <Common/Memory.h>
#include <Common/Utility.h>
#include <RHI/RHI.h>

namespace Render {
    // gpu geometry for a single static mesh lod, vertex layout matches StaticMeshVertexFactory (position + uv0
    // interleaved), created and destroyed on the render thread and shared between scene proxies
    class MeshRenderData {
    public:
        struct Vertex {
            Common::FVec3 position;
            Common::FVec2 uv0;
        };

        static constexpr size_t vertexStride = sizeof(Vertex);

        MeshRenderData(RHI::Device& inDevice, const std::vector<Vertex>& inVertices, const std::vector<uint32_t>& inIndices);
        ~MeshRenderData();

        NonCopyable(MeshRenderData)
        NonMovable(MeshRenderData)

        RHI::Buffer* GetVertexBuffer() const;
        RHI::Buffer* GetIndexBuffer() const;
        uint32_t GetIndexCount() const;

    private:
        Common::UniquePtr<RHI::Buffer> vertexBuffer;
        Common::UniquePtr<RHI::Buffer> indexBuffer;
        uint32_t indexCount;
    };
}

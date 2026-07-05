//
// Created by johnk on 2025/3/24.
//

#pragma once

#include <Common/Math/Matrix.h>
#include <Common/Math/Vector.h>
#include <Common/Memory.h>
#include <Render/MeshRenderData.h>

namespace Render {
    class MaterialShaderType;
    class VertexFactoryType;

    struct PrimitiveSceneProxy {
        PrimitiveSceneProxy();

        Common::FMat4x4 localToWorld;
        Common::SharedPtr<MeshRenderData> mesh;
        const VertexFactoryType* vertexFactoryType;
        const MaterialShaderType* vertexShaderType;
        const MaterialShaderType* pixelShaderType;
        Common::FVec4 baseColor;
    };
}

namespace Render {
    inline PrimitiveSceneProxy::PrimitiveSceneProxy()
        : localToWorld(Common::FMat4x4Consts::identity)
        , vertexFactoryType(nullptr)
        , vertexShaderType(nullptr)
        , pixelShaderType(nullptr)
        , baseColor(1.0f, 1.0f, 1.0f, 1.0f)
    {
    }
}

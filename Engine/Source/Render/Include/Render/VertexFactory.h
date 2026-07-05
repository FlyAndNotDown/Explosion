//
// Created by johnk on 2026/7/5.
//

#pragma once

#include <Render/Shader.h>

namespace Render {
    class StaticMeshVertexFactory final : public StaticVertexFactoryType<StaticMeshVertexFactory> {
    public:
        VertexFactoryTypeInfo(StaticMeshVertexFactory, "Engine/Shader/Explosion/VertexFactory/StaticMesh/VertexFactory.esh")
        DeclVertexInput(PositionInput, POSITION, RHI::VertexFormat::float32X3, 0)
        DeclVertexInput(Uv0Input, TEXCOORD, RHI::VertexFormat::float32X2, 12)
        MakeVertexInputVec(PositionInput, Uv0Input)
        EmptyVariantFieldVec
        BeginSupportedMaterialTypes
            MaterialType::surface,
        EndSupportedMaterialTypes
    };
}

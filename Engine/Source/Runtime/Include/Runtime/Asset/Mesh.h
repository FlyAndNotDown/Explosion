//
// Created by johnk on 2025/3/21.
//

#pragma once

#include <Common/Math/Adapters.h>
#include <Runtime/Asset/Asset.h>
#include <Runtime/Asset/Material.h>
#include <Runtime/Meta.h>
#include <Runtime/Api.h>

namespace Runtime {
    struct RUNTIME_API EClass() StaticMeshVertices {
        EClassBody(MeshVerticesData)

        EProperty() uint32_t vertexCount;
        EProperty() uint32_t indexCount;
        EProperty() std::vector<Common::FVec3> positions;
        EProperty() std::vector<Common::FVec3> tangents;
        EProperty() std::vector<Common::FVec2> uv0;
        EProperty() std::vector<uint32_t> indices;
        // optional
        EProperty() std::vector<Common::FVec2> uv1;
        EProperty() std::vector<Common::FVec3> colors;
    };

    struct RUNTIME_API EClass() StaticMeshLOD {
        EClassBody(MeshLOD)

        EProperty() StaticMeshVertices vertices;
        // TODO distance field data ?
        // TODO voxel data ?
    };

    class RUNTIME_API EClass() StaticMesh final : public Asset {
        EPolyDerivedClassBody(StaticMesh)

    public:
        explicit StaticMesh(Core::Uri inUri);
        ~StaticMesh() override;

        EFunc() const AssetPtr<MaterialInstance>& GetMaterial() const;
        EFunc() void SetMaterial(const AssetPtr<MaterialInstance>& inMaterial);
        EFunc() size_t GetLODCount() const;
        EFunc() const StaticMeshLOD& GetLOD(size_t inIndex) const;
        EFunc() StaticMeshLOD& EmplaceLOD();

    private:
        EProperty() AssetPtr<MaterialInstance> material;
        EProperty() std::vector<StaticMeshLOD> lodVec;
    };
}

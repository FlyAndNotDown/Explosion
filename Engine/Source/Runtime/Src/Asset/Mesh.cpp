//
// Created by johnk on 2025/3/21.
//

#include <Runtime/Asset/Mesh.h>

namespace Runtime {
    StaticMesh::StaticMesh(Core::Uri inUri)
        : Asset(std::move(inUri))
    {
    }

    StaticMesh::~StaticMesh() = default;

    const AssetPtr<MaterialInstance>& StaticMesh::GetMaterial() const
    {
        return material;
    }

    void StaticMesh::SetMaterial(const AssetPtr<MaterialInstance>& inMaterial)
    {
        material = inMaterial;
    }

    size_t StaticMesh::GetLODCount() const
    {
        return lodVec.size();
    }

    const StaticMeshLOD& StaticMesh::GetLOD(size_t inIndex) const
    {
        return lodVec.at(inIndex);
    }

    StaticMeshLOD& StaticMesh::EmplaceLOD()
    {
        return lodVec.emplace_back();
    }
}

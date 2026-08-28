//
// Created by johnk on 16/3/2022.
//

#include <RHI/DirectX12/ShaderModule.h>

namespace RHI::DirectX12 {
    DX12ShaderModule::DX12ShaderModule(const ShaderModuleCreateInfo& inCreateInfo)
        : ShaderModule(inCreateInfo)
        , byteCode(inCreateInfo.byteCode)
        , entryPoint(inCreateInfo.entryPoint)
    {
        Assert(byteCode != nullptr);
    }

    DX12ShaderModule::~DX12ShaderModule() = default;

    const std::string& DX12ShaderModule::GetEntryPoint()
    {
        return entryPoint;
    }

    D3D12_SHADER_BYTECODE DX12ShaderModule::GetNative() const
    {
        return CD3DX12_SHADER_BYTECODE(byteCode->GetData(), byteCode->GetSize());
    }
}

//
// Created by johnk on 2023/8/7.
//

#include <RHI/DirectX12/DX12RHIModule.h>
#include <RHI/DirectX12/Instance.h>

namespace RHI::DirectX12 {
    DX12RHIModule::DX12RHIModule() = default;

    DX12RHIModule::~DX12RHIModule() = default;

    void DX12RHIModule::OnLoad()
    {
        RHIModule::OnLoad();
    }

    void DX12RHIModule::OnUnload()
    {
        auto* instance = gInstance;
        gInstance = nullptr;
        delete instance;
    }

    Core::ModuleType DX12RHIModule::Type() const
    {
        return Core::ModuleType::mDynamic;
    }

    Instance* DX12RHIModule::GetRHIInstance() // NOLINT
    {
        return gInstance;
    }

    Instance* DX12RHIModule::CreateRHIInstance(const InstanceCreateInfo& inCreateInfo)
    {
        Assert(gInstance == nullptr);
        gInstance = new DX12Instance(inCreateInfo);
        return gInstance;
    }
}

IMPLEMENT_DYNAMIC_MODULE(RHI_DIRECTX12_API, RHI::DirectX12::DX12RHIModule);

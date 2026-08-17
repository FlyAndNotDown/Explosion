//
// Created by johnk on 2023/8/7.
//

#include <RHI/Vulkan/VulkanRHIModule.h>
#include <RHI/Vulkan/Instance.h>

namespace RHI::Vulkan {
    VulkanRHIModule::VulkanRHIModule() = default;

    VulkanRHIModule::~VulkanRHIModule() = default;

    void VulkanRHIModule::OnLoad()
    {
        RHIModule::OnLoad();
    }

    void VulkanRHIModule::OnUnload()
    {
        auto* instance = gInstance;
        gInstance = nullptr;
        delete instance;
    }

    Core::ModuleType VulkanRHIModule::Type() const
    {
        return Core::ModuleType::mDynamic;
    }

    Instance* VulkanRHIModule::GetRHIInstance() // NOLINT
    {
        return gInstance;
    }

    Instance* VulkanRHIModule::CreateRHIInstance(const InstanceCreateInfo& inCreateInfo)
    {
        Assert(gInstance == nullptr);
        gInstance = new VulkanInstance(inCreateInfo);
        return gInstance;
    }
}

IMPLEMENT_DYNAMIC_MODULE(RHI_VULKAN_API, RHI::Vulkan::VulkanRHIModule);

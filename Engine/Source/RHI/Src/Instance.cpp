//
// Created by johnk on 9/1/2022.
//

#include <RHI/Instance.h>
#include <RHI/RHIModule.h>

namespace RHI {
    InstanceCreateInfo::InstanceCreateInfo()
#if BUILD_CONFIG_DEBUG
        : gpuDebug(false)
#endif
    {
    }

    RHIType GetPlatformRHIType()
    {
#if PLATFORM_WINDOWS
        return RHIType::directX12;
#else
        return RHIType::vulkan;
#endif
    }

    std::string GetPlatformDefaultRHIAbbrString()
    {
#if PLATFORM_WINDOWS
        return "dx12";
#else
        return "vulkan";
#endif
    }

    std::string GetAbbrStringByType(RHIType type)
    {
        static std::unordered_map<RHIType, std::string> map = {
            { RHIType::directX12, "dx12" },
            { RHIType::vulkan, "vulkan" },
            { RHIType::dummy, "dummy" }
        };
        return map.at(type);
    }

    RHIType GetRHITypeByAbbrString(const std::string& abbrString) // NOLINT
    {
        static std::unordered_map<std::string, RHIType> map = {
            { "dx12", RHIType::directX12 },
            { "vulkan", RHIType::vulkan },
            { "dummy", RHIType::dummy }
        };
        return map.at(abbrString);
    }

    std::string GetRHIModuleNameByType(const RHIType type)
    {
        static std::unordered_map<RHIType, std::string> map = {
            { RHIType::directX12, "RHI-DirectX12" },
            { RHIType::vulkan, "RHI-Vulkan" },
            { RHIType::dummy, "RHI-Dummy" }
        };
        return map.at(type);
    }

    Instance* Instance::GetByPlatform(const InstanceCreateInfo& inCreateInfo)
    {
        return GetByType(GetPlatformRHIType(), inCreateInfo);
    }

    Instance* Instance::GetByType(const RHIType& type, const InstanceCreateInfo& inCreateInfo)
    {
        auto* module = Core::ModuleManager::Get().FindOrLoadTyped<RHIModule>(GetRHIModuleNameByType(type));
        if (module == nullptr) {
            return nullptr;
        }
        auto* instance = module->GetRHIInstance();
        if (instance == nullptr) {
            instance = module->CreateRHIInstance(inCreateInfo);
        } else {
#if BUILD_CONFIG_DEBUG
            Assert(instance->GetCreateInfo().gpuDebug == inCreateInfo.gpuDebug);
#endif
        }
        return instance;
    }

    void Instance::UnloadByType(const RHIType& type)
    {
        Core::ModuleManager::Get().Unload(GetRHIModuleNameByType(type));
    }

    void Instance::UnloadAllInstances()
    {
        for (auto i = static_cast<uint32_t>(RHIType::directX12); i < static_cast<uint32_t>(RHIType::max); i++) {
            const auto rhiType = static_cast<RHIType>(i);
            Core::ModuleManager::Get().Unload(GetRHIModuleNameByType(rhiType));
        }
    }

    Instance::~Instance() = default;

    Instance::Instance(const InstanceCreateInfo& inCreateInfo)
        : createInfo(inCreateInfo)
    {
    }

    const InstanceCreateInfo& Instance::GetCreateInfo() const
    {
        return createInfo;
    }
}

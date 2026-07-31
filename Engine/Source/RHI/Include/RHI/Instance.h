//
// Created by johnk on 9/1/2022.
//

#pragma once

#include <cstdint>

#include <Common/Utility.h>

#include <RHI/Common.h>

namespace RHI {
    class Gpu;

    struct InstanceCreateInfo {
        InstanceCreateInfo();

#if BUILD_CONFIG_DEBUG
        bool gpuDebug;
#endif
    };

    RHIType GetPlatformRHIType();
    std::string GetPlatformDefaultRHIAbbrString();
    std::string GetAbbrStringByType(RHIType type);
    RHIType GetRHITypeByAbbrString(const std::string& abbrString);
    std::string GetRHIModuleNameByType(RHIType type);

    class Instance {
    public:
        static Instance* GetByPlatform(const InstanceCreateInfo& inCreateInfo = {});
        static Instance* GetByType(const RHIType& type, const InstanceCreateInfo& inCreateInfo = {});
        static void UnloadByType(const RHIType& type);
        static void UnloadAllInstances();

        NonCopyable(Instance)
        virtual ~Instance();
        const InstanceCreateInfo& GetCreateInfo() const;
        virtual RHIType GetRHIType() = 0;
        virtual uint32_t GetGpuNum() = 0;
        virtual Gpu* GetGpu(uint32_t index) = 0;
        virtual void Destroy() = 0;

    protected:
        explicit Instance(const InstanceCreateInfo& inCreateInfo);

    private:
        InstanceCreateInfo createInfo;
    };
}

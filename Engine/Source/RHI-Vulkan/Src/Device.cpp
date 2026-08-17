//
// Created by johnk on 16/1/2022.
//

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <span>

#include <RHI/Vulkan/Common.h>
#include <RHI/Vulkan/Instance.h>
#include <RHI/Vulkan/Gpu.h>
#include <RHI/Vulkan/Device.h>
#include <RHI/Vulkan/Queue.h>
#include <RHI/Vulkan/Buffer.h>
#include <RHI/Vulkan/ShaderModule.h>
#include <RHI/Vulkan/PipelineLayout.h>
#include <RHI/Vulkan/BindGroupLayout.h>
#include <RHI/Vulkan/BindGroup.h>
#include <RHI/Vulkan/Sampler.h>
#include <RHI/Vulkan/Texture.h>
#include <RHI/Vulkan/SwapChain.h>
#include <RHI/Vulkan/Pipeline.h>
#include <RHI/Vulkan/CommandBuffer.h>
#include <RHI/Vulkan/Synchronous.h>
#include <RHI/Vulkan/Surface.h>
#include <RHI/Vulkan/QuerySet.h>
#include <RHI/Vulkan/PipelineCache.h>

namespace RHI::Vulkan {
    const std::vector requiredExtensions = {
        "VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering",
        "VK_KHR_depth_stencil_resolve",
        "VK_KHR_create_renderpass2",
#if PLATFORM_MACOS
        "VK_KHR_portability_subset",
        "VK_EXT_extended_dynamic_state"
#endif
    };

}

namespace RHI::Vulkan::Internal {
    struct QueueFamilyRule {
        VkQueueFlags requiredFlags;
        VkQueueFlags excludedFlags;
    };

    constexpr std::array graphicsQueueFamilyRules = {
        QueueFamilyRule { VK_QUEUE_GRAPHICS_BIT, 0 }
    };

    constexpr std::array computeQueueFamilyRules = {
        QueueFamilyRule { VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT },
        QueueFamilyRule { VK_QUEUE_COMPUTE_BIT, 0 }
    };

    constexpr std::array transferQueueFamilyRules = {
        QueueFamilyRule { VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT },
        QueueFamilyRule { VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT },
        QueueFamilyRule { VK_QUEUE_GRAPHICS_BIT, 0 }
    };

    static std::span<const QueueFamilyRule> GetQueueFamilyRules(const QueueType inQueueType)
    {
        if (inQueueType == QueueType::graphics) {
            return graphicsQueueFamilyRules;
        }
        if (inQueueType == QueueType::compute) {
            return computeQueueFamilyRules;
        }
        if (inQueueType == QueueType::transfer) {
            return transferQueueFamilyRules;
        }
        Unimplement();
        return {};
    }

    static std::optional<uint32_t> FindQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& inProperties, const QueueType inQueueType)
    {
        for (const auto& rule : GetQueueFamilyRules(inQueueType)) {
            for (uint32_t i = 0; i < inProperties.size(); i++) {
                const auto& properties = inProperties[i];
                if (properties.queueCount > 0
                    && (properties.queueFlags & rule.requiredFlags) == rule.requiredFlags
                    && (properties.queueFlags & rule.excludedFlags) == 0) {
                    return i;
                }
            }
        }
        return {};
    }
}

namespace RHI::Vulkan {
    VulkanDevice::VulkanDevice(VulkanGpu& inGpu, const DeviceCreateInfo& inCreateInfo)
        : Device(inCreateInfo)
        , gpu(inGpu)
    {
        CreateNativeDevice(inCreateInfo);
        GetQueues();
        CreateNativeVmaAllocator();
    }

    VulkanDevice::~VulkanDevice()
    {
        vmaDestroyAllocator(nativeAllocator);

        for (auto& [queue, pool] : nativeCmdPools) {
            vkDestroyCommandPool(nativeDevice, pool, nullptr);
        }
        vkDestroyDevice(nativeDevice, nullptr);
    }

    VulkanGpu& VulkanDevice::GetGpu() const
    {
        return gpu;
    }

    size_t VulkanDevice::GetQueueNum(const QueueType inType)
    {
        return queues.at(inType).size();
    }

    Queue* VulkanDevice::GetQueue(QueueType inType, size_t inIndex)
    {
        const auto& queueArray = queues.at(inType);
        Assert(inIndex < queueArray.size());
        return queueArray[inIndex].Get();
    }

    Common::UniquePtr<Surface> VulkanDevice::CreateSurface(const SurfaceCreateInfo& inCreateInfo)
    {
        return { new VulkanSurface(*this, inCreateInfo) };
    }

    Common::UniquePtr<SwapChain> VulkanDevice::CreateSwapChain(const SwapChainCreateInfo& inCreateInfo)
    {
        return { new VulkanSwapChain(*this, inCreateInfo) };
    }

    Common::UniquePtr<Buffer> VulkanDevice::CreateBuffer(const BufferCreateInfo& inCreateInfo)
    {
        return { new VulkanBuffer(*this, inCreateInfo) };
    }

    Common::UniquePtr<Texture> VulkanDevice::CreateTexture(const TextureCreateInfo& inCreateInfo)
    {
        return { new VulkanTexture(*this, inCreateInfo) };
    }

    Common::UniquePtr<Sampler> VulkanDevice::CreateSampler(const SamplerCreateInfo& inCreateInfo)
    {
        return { new VulkanSampler(*this, inCreateInfo) };
    }

    Common::UniquePtr<BindGroupLayout> VulkanDevice::CreateBindGroupLayout(const BindGroupLayoutCreateInfo& inCreateInfo)
    {
        return { new VulkanBindGroupLayout(*this, inCreateInfo) };
    }

    Common::UniquePtr<BindGroup> VulkanDevice::CreateBindGroup(const BindGroupCreateInfo& inCreateInfo)
    {
        return { new VulkanBindGroup(*this, inCreateInfo) };
    }

    Common::UniquePtr<PipelineLayout> VulkanDevice::CreatePipelineLayout(const PipelineLayoutCreateInfo& inCreateInfo)
    {
        return { new VulkanPipelineLayout(*this, inCreateInfo) };
    }

    Common::UniquePtr<ShaderModule> VulkanDevice::CreateShaderModule(const ShaderModuleCreateInfo& inCreateInfo)
    {
        return { new VulkanShaderModule(*this, inCreateInfo) };
    }

    Common::UniquePtr<ComputePipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineCreateInfo& inCreateInfo)
    {
        return { new VulkanComputePipeline(*this, inCreateInfo) };
    }

    Common::UniquePtr<RasterPipeline> VulkanDevice::CreateRasterPipeline(const RasterPipelineCreateInfo& inCreateInfo)
    {
        return { new VulkanRasterPipeline(*this, inCreateInfo) };
    }

    Common::UniquePtr<CommandBuffer> VulkanDevice::CreateCommandBuffer(const QueueType inQueueType)
    {
        Assert(nativeCmdPools.contains(inQueueType));
        return { new VulkanCommandBuffer(*this, inQueueType, nativeCmdPools.at(inQueueType)) };
    }

    Common::UniquePtr<Fence> VulkanDevice::CreateFence(const bool initAsSignaled)
    {
        return { new VulkanFence(*this, initAsSignaled) };
    }

    Common::UniquePtr<Semaphore> VulkanDevice::CreateSemaphore()
    {
        return { new VulkanSemaphore(*this) };
    }

    Common::UniquePtr<QuerySet> VulkanDevice::CreateQuerySet(const QuerySetCreateInfo& inCreateInfo)
    {
        return { new VulkanQuerySet(*this, inCreateInfo) };
    }

    Common::UniquePtr<PipelineCache> VulkanDevice::CreatePipelineCache(const PipelineCacheCreateInfo& inCreateInfo)
    {
        return { new VulkanPipelineCache(*this, inCreateInfo) };
    }

    bool VulkanDevice::CheckSwapChainFormatSupport(Surface* inSurface, const PixelFormat inFormat, const ColorSpace inColorSpace)
    {
        const auto* vkSurface = static_cast<VulkanSurface*>(inSurface);

        uint32_t formatCount = 0;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.GetNative(), vkSurface->GetNative(), &formatCount, nullptr);
        Assert(formatCount != 0);
        surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.GetNative(), vkSurface->GetNative(), &formatCount, surfaceFormats.data());

        const auto iter = std::ranges::find_if(
            surfaceFormats,
            [format = EnumCast<PixelFormat, VkFormat>(inFormat), colorSpace = EnumCast<ColorSpace, VkColorSpaceKHR>(inColorSpace)](const VkSurfaceFormatKHR surfaceFormat) {
                return format == surfaceFormat.format && colorSpace == surfaceFormat.colorSpace;
            });
        return iter != surfaceFormats.end();
    }

    TextureSubResourceCopyFootprint VulkanDevice::GetTextureSubResourceCopyFootprint(const Texture& texture, const TextureSubResourceInfo& subResourceInfo, const Common::UVec3& copyRegion)
    {
        const auto& createInfo = texture.GetCreateInfo();
        const auto mipLevel = subResourceInfo.mipLevel;
        const auto baseDepth = createInfo.type == TextureType::t3D ? createInfo.depthOrArraySize : 1;
        const Common::UVec3 subResourceExtent = {
            std::max(createInfo.width >> mipLevel, 1u),
            std::max(createInfo.height >> mipLevel, 1u),
            std::max(baseDepth >> mipLevel, 1u)
        };
        const auto useFullSubResource = copyRegion == Common::UVec3Consts::zero;
        const auto extent = useFullSubResource ? subResourceExtent : copyRegion;
        Assert(extent.x <= subResourceExtent.x && extent.y <= subResourceExtent.y && extent.z <= subResourceExtent.z);

        TextureSubResourceCopyFootprint result {};
        result.extent = extent;
        result.bytesPerPixel = GetBytesPerPixel(createInfo.format);
        result.rowPitch = result.bytesPerPixel * result.extent.x;
        result.slicePitch = result.rowPitch * result.extent.y;
        result.totalBytes = result.slicePitch * result.extent.z;
        return result;
    }

    VkDevice VulkanDevice::GetNative() const
    {
        return nativeDevice;
    }

    const std::vector<uint32_t>& VulkanDevice::GetActiveQueueFamilyIndices() const
    {
        return activeQueueFamilyIndices;
    }

    const VkPhysicalDeviceFeatures& VulkanDevice::GetEnabledFeatures() const
    {
        return enabledFeatures;
    }

    void VulkanDevice::CreateNativeDevice(const DeviceCreateInfo& inCreateInfo)
    {
        uint32_t queueFamilyPropertyCnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu.GetNative(), &queueFamilyPropertyCnt, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCnt);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu.GetNative(), &queueFamilyPropertyCnt, queueFamilyProperties.data());

        std::map<QueueType, uint32_t> queueNumMap;
        for (uint32_t i = 0; i < inCreateInfo.queueRequests.size(); i++) {
            const auto& queueCreateInfo = inCreateInfo.queueRequests[i];
            if (auto iter = queueNumMap.find(queueCreateInfo.type);
                iter == queueNumMap.end()) {
                queueNumMap[queueCreateInfo.type] = 0;
            }
            queueNumMap[queueCreateInfo.type] += queueCreateInfo.num;
        }

        std::map<uint32_t, uint32_t> familyQueueCounts;
        std::map<uint32_t, uint32_t> familySharedQueueCursors;
        for (auto [queueType, queueNum] : queueNumMap) {
            auto queueFamilyIndex = Internal::FindQueueFamilyIndex(queueFamilyProperties, queueType);
            Assert(queueFamilyIndex.has_value());
            const auto familyIndex = queueFamilyIndex.value();
            const auto familyQueueCapacity = queueFamilyProperties[familyIndex].queueCount;
            const auto queueCount = std::min(familyQueueCapacity, queueNum);
            Assert(queueCount > 0);

            QueueFamilyMapping mapping;
            mapping.familyIndex = familyIndex;
            mapping.queueIndices.reserve(queueCount);
            for (uint32_t i = 0; i < queueCount; i++) {
                auto& allocatedQueueCount = familyQueueCounts[familyIndex];
                if (allocatedQueueCount < familyQueueCapacity) {
                    mapping.queueIndices.emplace_back(allocatedQueueCount);
                    allocatedQueueCount++;
                } else {
                    auto& sharedQueueCursor = familySharedQueueCursors[familyIndex];
                    mapping.queueIndices.emplace_back(sharedQueueCursor % familyQueueCapacity);
                    sharedQueueCursor++;
                }
            }
            queueFamilyMappings.emplace(queueType, std::move(mapping));
        }

        activeQueueFamilyIndices.clear();
        activeQueueFamilyIndices.reserve(familyQueueCounts.size());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(familyQueueCounts.size());
        std::vector<std::vector<float>> queuePriorities;
        queuePriorities.reserve(familyQueueCounts.size());
        for (const auto [familyIndex, queueCount] : familyQueueCounts) {
            activeQueueFamilyIndices.emplace_back(familyIndex);
            queuePriorities.emplace_back(queueCount, 1.0f);

            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = familyIndex;
            queueCreateInfo.queueCount = queueCount;
            queueCreateInfo.pQueuePriorities = queuePriorities.back().data();
            queueCreateInfos.emplace_back(queueCreateInfo);
        }

        VkPhysicalDeviceDynamicRenderingFeatures supportedDynamicRenderingFeatures = {};
        supportedDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT supportedExtendedDynamicStateFeatures = {};
        supportedExtendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
        supportedDynamicRenderingFeatures.pNext = &supportedExtendedDynamicStateFeatures;

        VkPhysicalDeviceFeatures2 supportedFeatures = {};
        supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supportedFeatures.pNext = &supportedDynamicRenderingFeatures;
        vkGetPhysicalDeviceFeatures2(gpu.GetNative(), &supportedFeatures);

        if (supportedDynamicRenderingFeatures.dynamicRendering != VK_TRUE) {
            QuickFailWithReason("required vulkan dynamic rendering feature is not supported");
        }
        if (supportedExtendedDynamicStateFeatures.extendedDynamicState != VK_TRUE) {
            QuickFailWithReason("required vulkan extended dynamic state feature is not supported");
        }

        enabledFeatures = {};
        enabledFeatures.independentBlend = supportedFeatures.features.independentBlend;
        enabledFeatures.multiDrawIndirect = supportedFeatures.features.multiDrawIndirect;
        enabledFeatures.drawIndirectFirstInstance = supportedFeatures.features.drawIndirectFirstInstance;
        enabledFeatures.fillModeNonSolid = supportedFeatures.features.fillModeNonSolid;
        enabledFeatures.depthClamp = supportedFeatures.features.depthClamp;
        enabledFeatures.depthBiasClamp = supportedFeatures.features.depthBiasClamp;
        enabledFeatures.samplerAnisotropy = supportedFeatures.features.samplerAnisotropy;
        enabledFeatures.textureCompressionBC = supportedFeatures.features.textureCompressionBC;
        enabledFeatures.occlusionQueryPrecise = supportedFeatures.features.occlusionQueryPrecise;
        enabledFeatures.imageCubeArray = supportedFeatures.features.imageCubeArray;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
        deviceCreateInfo.pNext = &dynamicRenderingFeatures;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {};
        extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
        extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;
        dynamicRenderingFeatures.pNext = &extendedDynamicStateFeatures;

        deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());

        Assert(vkCreateDevice(gpu.GetNative(), &deviceCreateInfo, nullptr, &nativeDevice) == VK_SUCCESS);
    }

    void VulkanDevice::GetQueues()
    {
        std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<std::mutex>> nativeQueueMutexes;

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        for (const auto& [queueType, queueFamilyInfo] : queueFamilyMappings) {
            const auto queueFamilyIndex = queueFamilyInfo.familyIndex;
            const auto& queueIndices = queueFamilyInfo.queueIndices;

            std::vector<Common::UniquePtr<VulkanQueue>> tempQueues(queueIndices.size());
            for (auto i = 0; i < tempQueues.size(); i++) {
                const auto queueIndex = queueIndices[i];
                VkQueue queue;
                vkGetDeviceQueue(nativeDevice, queueFamilyIndex, queueIndex, &queue);
                const auto queueKey = std::make_pair(queueFamilyIndex, queueIndex);
                auto& queueMutex = nativeQueueMutexes[queueKey];
                if (queueMutex == nullptr) {
                    queueMutex = std::make_shared<std::mutex>();
                }
                tempQueues[i] = Common::MakeUnique<VulkanQueue>(*this, queueType, queue, queueMutex);
            }
            queues[queueType] = std::move(tempQueues);

            poolInfo.queueFamilyIndex = queueFamilyIndex;

            VkCommandPool pool;
            Assert(vkCreateCommandPool(nativeDevice, &poolInfo, nullptr, &pool) == VK_SUCCESS);
            nativeCmdPools.emplace(queueType, pool);
        }
    }

    void VulkanDevice::CreateNativeVmaAllocator()
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo info = {};
        info.vulkanApiVersion = VK_API_VERSION_1_3;
        info.instance = gpu.GetInstance().GetNative();
        info.physicalDevice = gpu.GetNative();
        info.device = nativeDevice;
        info.pVulkanFunctions = &vulkanFunctions;

        vmaCreateAllocator(&info, &nativeAllocator);
    }

    VmaAllocator& VulkanDevice::GetNativeAllocator()
    {
        return nativeAllocator;
    }

#if BUILD_CONFIG_DEBUG
    void VulkanDevice::SetObjectName(const VkObjectType inObjectType, const uint64_t inObjectHandle, const char* inObjectName) const
    {
        VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        info.objectType                    = inObjectType;
        info.objectHandle                  = inObjectHandle;
        info.pObjectName                   = inObjectName;

        auto* pfn = gpu.GetInstance().FindOrGetTypedDynamicFuncPointer<PFN_vkSetDebugUtilsObjectNameEXT>("vkSetDebugUtilsObjectNameEXT");
        pfn(nativeDevice, &info);
    }
#endif
}

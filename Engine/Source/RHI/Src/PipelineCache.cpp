//
// Created by johnk on 2026/7/2.
//

#include <utility>

#include <RHI/PipelineCache.h>

namespace RHI {
    PipelineCacheCreateInfo::PipelineCacheCreateInfo()
        : initialData(nullptr)
        , initialDataSize(0)
    {
    }

    PipelineCacheCreateInfo::PipelineCacheCreateInfo(const void* inInitialData, const size_t inInitialDataSize, std::string inDebugName)
        : initialData(inInitialData)
        , initialDataSize(inInitialDataSize)
        , debugName(std::move(inDebugName))
    {
    }

    PipelineCacheCreateInfo::PipelineCacheCreateInfo(const std::vector<uint8_t>& inInitialData, std::string inDebugName)
        : initialData(inInitialData.data())
        , initialDataSize(inInitialData.size())
        , debugName(std::move(inDebugName))
    {
    }

    PipelineCacheCreateInfo& PipelineCacheCreateInfo::SetInitialData(const void* inInitialData)
    {
        initialData = inInitialData;
        return *this;
    }

    PipelineCacheCreateInfo& PipelineCacheCreateInfo::SetInitialDataSize(const size_t inInitialDataSize)
    {
        initialDataSize = inInitialDataSize;
        return *this;
    }

    PipelineCacheCreateInfo& PipelineCacheCreateInfo::SetDebugName(std::string inDebugName)
    {
        debugName = std::move(inDebugName);
        return *this;
    }

    PipelineCache::PipelineCache(const PipelineCacheCreateInfo&) {}

    PipelineCache::~PipelineCache() = default;
}

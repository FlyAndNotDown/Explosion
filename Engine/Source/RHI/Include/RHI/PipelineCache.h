//
// Created by johnk on 2026/7/2.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Common/Utility.h>

namespace RHI {
    struct PipelineCacheCreateInfo {
        const void* initialData;
        size_t initialDataSize;
        std::string debugName;

        PipelineCacheCreateInfo();
        explicit PipelineCacheCreateInfo(const void* inInitialData, size_t inInitialDataSize, std::string inDebugName = "");
        explicit PipelineCacheCreateInfo(const std::vector<uint8_t>& inInitialData, std::string inDebugName = "");

        PipelineCacheCreateInfo& SetInitialData(const void* inInitialData);
        PipelineCacheCreateInfo& SetInitialDataSize(size_t inInitialDataSize);
        PipelineCacheCreateInfo& SetDebugName(std::string inDebugName);
    };

    class PipelineCache {
    public:
        NonCopyable(PipelineCache)
        virtual ~PipelineCache();

        virtual std::vector<uint8_t> GetData() = 0;

    protected:
        explicit PipelineCache(const PipelineCacheCreateInfo& inCreateInfo);
    };
}

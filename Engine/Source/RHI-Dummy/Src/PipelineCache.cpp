//
// Created by johnk on 2026/7/2.
//

#include <RHI/Dummy/PipelineCache.h>

namespace RHI::Dummy {
    DummyPipelineCache::DummyPipelineCache(const PipelineCacheCreateInfo& createInfo)
        : PipelineCache(createInfo)
    {
    }

    DummyPipelineCache::~DummyPipelineCache() = default;

    std::vector<uint8_t> DummyPipelineCache::GetData()
    {
        return {};
    }
}

//
// Created by johnk on 2026/7/2.
//

#pragma once

#include <RHI/PipelineCache.h>

namespace RHI::Dummy {
    class DummyPipelineCache final : public PipelineCache {
    public:
        NonCopyable(DummyPipelineCache)
        explicit DummyPipelineCache(const PipelineCacheCreateInfo& createInfo);
        ~DummyPipelineCache() override;

        std::vector<uint8_t> GetData() override;
    };
}

//
// Created by johnk on 2026/6/30.
//

#pragma once

#include <RHI/QuerySet.h>

namespace RHI::Dummy {
    class DummyQuerySet final : public QuerySet {
    public:
        NonCopyable(DummyQuerySet)
        explicit DummyQuerySet(const QuerySetCreateInfo& createInfo);
        ~DummyQuerySet() override;
    };
}

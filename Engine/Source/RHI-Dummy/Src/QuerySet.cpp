//
// Created by johnk on 2026/6/30.
//

#include <RHI/Dummy/QuerySet.h>

namespace RHI::Dummy {
    DummyQuerySet::DummyQuerySet(const QuerySetCreateInfo& createInfo)
        : QuerySet(createInfo)
    {
    }

    DummyQuerySet::~DummyQuerySet() = default;
}

//
// Created by johnk on 2026/6/30.
//

#include <RHI/QuerySet.h>

namespace RHI {
    QuerySetCreateInfo::QuerySetCreateInfo()
        : type(QueryType::max)
        , count(0)
    {
    }

    QuerySetCreateInfo::QuerySetCreateInfo(const QueryType inType, const uint32_t inCount, std::string inDebugName)
        : type(inType)
        , count(inCount)
        , debugName(std::move(inDebugName))
    {
    }

    QuerySetCreateInfo& QuerySetCreateInfo::SetType(const QueryType inType)
    {
        type = inType;
        return *this;
    }

    QuerySetCreateInfo& QuerySetCreateInfo::SetCount(const uint32_t inCount)
    {
        count = inCount;
        return *this;
    }

    QuerySetCreateInfo& QuerySetCreateInfo::SetDebugName(std::string inDebugName)
    {
        debugName = std::move(inDebugName);
        return *this;
    }

    QuerySet::QuerySet(const QuerySetCreateInfo& inCreateInfo)
        : createInfo(inCreateInfo)
    {
    }

    QuerySet::~QuerySet() = default;

    const QuerySetCreateInfo& QuerySet::GetCreateInfo() const
    {
        return createInfo;
    }
}

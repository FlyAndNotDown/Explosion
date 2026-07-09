//
// Created by johnk on 2026/6/30.
//

#pragma once

#include <string>

#include <Common/Utility.h>
#include <RHI/Common.h>

namespace RHI {
    struct QuerySetCreateInfo {
        QueryType type;
        uint32_t count;
        std::string debugName;

        QuerySetCreateInfo();
        QuerySetCreateInfo(QueryType inType, uint32_t inCount, std::string inDebugName = "");

        QuerySetCreateInfo& SetType(QueryType inType);
        QuerySetCreateInfo& SetCount(uint32_t inCount);
        QuerySetCreateInfo& SetDebugName(std::string inDebugName);
    };

    class QuerySet {
    public:
        NonCopyable(QuerySet)
        virtual ~QuerySet();

        const QuerySetCreateInfo& GetCreateInfo() const;

    protected:
        explicit QuerySet(const QuerySetCreateInfo& inCreateInfo);

        QuerySetCreateInfo createInfo;
    };
}

//
// Created by johnk on 19/2/2022.
//

#include <cstring>
#include <utility>

#include <RHI/ShaderModule.h>

namespace RHI {
    ShaderByteCode::ShaderByteCode(const void* inData, const size_t inSize)
        : data(inSize)
    {
        Assert(inData != nullptr || inSize == 0);
        if (inSize > 0) {
            std::memcpy(data.data(), inData, inSize);
        }
    }

    ShaderByteCode::ShaderByteCode(const std::vector<uint8_t>& inData)
        : data(inData)
    {
    }

    ShaderByteCode::ShaderByteCode(std::vector<uint8_t>&& inData)
        : data(std::move(inData))
    {
    }

    ShaderByteCode::~ShaderByteCode() = default;

    const void* ShaderByteCode::GetData() const
    {
        return data.data();
    }

    size_t ShaderByteCode::GetSize() const
    {
        return data.size();
    }

    ShaderModuleCreateInfo::ShaderModuleCreateInfo(const std::string& inEntryPoint, ShaderByteCodeRef inByteCode)
        : entryPoint(inEntryPoint)
        , byteCode(std::move(inByteCode))
    {
    }

    ShaderModuleCreateInfo::ShaderModuleCreateInfo(const std::string& inEntryPoint, const void* inByteCode, const size_t inSize)
        : entryPoint(inEntryPoint)
        , byteCode(Common::MakeShared<const ShaderByteCode>(inByteCode, inSize))
    {
    }

    ShaderModuleCreateInfo::ShaderModuleCreateInfo(const std::string& inEntryPoint, const std::vector<uint8_t>& inByteCode)
        : entryPoint(inEntryPoint)
        , byteCode(Common::MakeShared<const ShaderByteCode>(inByteCode))
    {
    }

    ShaderModuleCreateInfo::ShaderModuleCreateInfo(const std::string& inEntryPoint, std::vector<uint8_t>&& inByteCode)
        : entryPoint(inEntryPoint)
        , byteCode(Common::MakeShared<const ShaderByteCode>(std::move(inByteCode)))
    {
    }

    ShaderModuleCreateInfo& ShaderModuleCreateInfo::SetByteCode(ShaderByteCodeRef inByteCode)
    {
        byteCode = std::move(inByteCode);
        return *this;
    }

    ShaderModuleCreateInfo& ShaderModuleCreateInfo::SetByteCode(const void* inByteCode, const size_t inSize)
    {
        byteCode = Common::MakeShared<const ShaderByteCode>(inByteCode, inSize);
        return *this;
    }

    ShaderModuleCreateInfo& ShaderModuleCreateInfo::SetByteCode(const std::vector<uint8_t>& inByteCode)
    {
        byteCode = Common::MakeShared<const ShaderByteCode>(inByteCode);
        return *this;
    }

    ShaderModuleCreateInfo& ShaderModuleCreateInfo::SetByteCode(std::vector<uint8_t>&& inByteCode)
    {
        byteCode = Common::MakeShared<const ShaderByteCode>(std::move(inByteCode));
        return *this;
    }

    ShaderModule::ShaderModule(const ShaderModuleCreateInfo&) {}

    ShaderModule::~ShaderModule() = default;
}

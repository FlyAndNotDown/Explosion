//
// Created by johnk on 19/2/2022.
//

#pragma once

#include <string>
#include <vector>

#include <Common/Memory.h>
#include <Common/Utility.h>

namespace RHI {
    class ShaderByteCode final {
    public:
        NonCopyable(ShaderByteCode)
        explicit ShaderByteCode(const void* inData, size_t inSize);
        explicit ShaderByteCode(const std::vector<uint8_t>& inData);
        explicit ShaderByteCode(std::vector<uint8_t>&& inData);
        ~ShaderByteCode();

        const void* GetData() const;
        size_t GetSize() const;

    private:
        std::vector<uint8_t> data;
    };

    using ShaderByteCodeRef = Common::SharedPtr<const ShaderByteCode>;

    struct ShaderModuleCreateInfo {
        std::string entryPoint;
        ShaderByteCodeRef byteCode;

        explicit ShaderModuleCreateInfo(const std::string& inEntryPoint = "", ShaderByteCodeRef inByteCode = {});
        explicit ShaderModuleCreateInfo(const std::string& inEntryPoint, const void* inByteCode, size_t inSize);
        explicit ShaderModuleCreateInfo(const std::string& inEntryPoint, const std::vector<uint8_t>& inByteCode);
        explicit ShaderModuleCreateInfo(const std::string& inEntryPoint, std::vector<uint8_t>&& inByteCode);

        ShaderModuleCreateInfo& SetByteCode(ShaderByteCodeRef inByteCode);
        ShaderModuleCreateInfo& SetByteCode(const void* inByteCode, size_t inSize);
        ShaderModuleCreateInfo& SetByteCode(const std::vector<uint8_t>& inByteCode);
        ShaderModuleCreateInfo& SetByteCode(std::vector<uint8_t>&& inByteCode);
    };

    class ShaderModule {
    public:
        NonCopyable(ShaderModule)
        virtual ~ShaderModule();

        virtual const std::string& GetEntryPoint() = 0;

    protected:
        explicit ShaderModule(const ShaderModuleCreateInfo& createInfo);
    };
}

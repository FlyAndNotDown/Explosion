//
// Created by johnk on 19/2/2022.
//

#pragma once

#include <cstdint>
#include <variant>

#include <Common/Utility.h>
#include <RHI/RHI.h>

namespace RHI {
    class BindGroupLayout;

    struct HlslPipelineConstantBinding {
        uint8_t binding;
        uint8_t bindingSpace;

        HlslPipelineConstantBinding(uint8_t inBinding, uint8_t inBindingSpace);
    };

    struct GlslPipelineConstantBinding {
        uint32_t offset;

        explicit GlslPipelineConstantBinding(uint32_t inOffset);
    };

    struct PipelineConstantLayout {
        ShaderStageFlags stageFlags;
        uint32_t size;
        std::variant<HlslPipelineConstantBinding, GlslPipelineConstantBinding> platformBinding;

        PipelineConstantLayout();
        PipelineConstantLayout(ShaderStageFlags inStageFlags, uint32_t inSize, const std::variant<HlslPipelineConstantBinding, GlslPipelineConstantBinding>& inPlatformBinding);
        PipelineConstantLayout& SetStageFlags(ShaderStageFlags inStageFlags);
        PipelineConstantLayout& SetSize(uint32_t inSize);
        PipelineConstantLayout& SetPlatformBinding(const std::variant<HlslPipelineConstantBinding, GlslPipelineConstantBinding>& inPlatformBinding);
    };

    struct PipelineLayoutCreateInfo {
        std::vector<const BindGroupLayout*> bindGroupLayouts;
        std::vector<PipelineConstantLayout> pipelineConstantLayouts;
        std::string debugName;

        PipelineLayoutCreateInfo();
        PipelineLayoutCreateInfo& AddBindGroupLayout(const BindGroupLayout* inLayout);
        PipelineLayoutCreateInfo& AddPipelineConstantLayout(const PipelineConstantLayout& inLayout);
    };

    class PipelineLayout {
    public:
        NonCopyable(PipelineLayout)
        virtual ~PipelineLayout();

    protected:
        explicit PipelineLayout(const PipelineLayoutCreateInfo& createInfo);
    };
}

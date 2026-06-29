//
// Created by johnk on 19/2/2022.
//

#include <RHI/PipelineLayout.h>

namespace RHI {
    HlslPipelineConstantBinding::HlslPipelineConstantBinding(const uint8_t inBinding, const uint8_t inBindingSpace)
        : binding(inBinding)
        , bindingSpace(inBindingSpace)
    {
    }

    GlslPipelineConstantBinding::GlslPipelineConstantBinding(const uint32_t inOffset)
        : offset(inOffset)
    {
    }

    PipelineConstantLayout::PipelineConstantLayout()
        : stageFlags(ShaderStageFlags::null)
        , size(0)
        , platformBinding(HlslPipelineConstantBinding(0, 0))
    {
    }

    PipelineConstantLayout::PipelineConstantLayout(const ShaderStageFlags inStageFlags, const uint32_t inSize, const std::variant<HlslPipelineConstantBinding, GlslPipelineConstantBinding>& inPlatformBinding)
        : stageFlags(inStageFlags)
        , size(inSize)
        , platformBinding(inPlatformBinding)
    {
    }

    PipelineConstantLayout& PipelineConstantLayout::SetStageFlags(const ShaderStageFlags inStageFlags)
    {
        stageFlags = inStageFlags;
        return *this;
    }

    PipelineConstantLayout& PipelineConstantLayout::SetSize(const uint32_t inSize)
    {
        size = inSize;
        return *this;
    }

    PipelineConstantLayout& PipelineConstantLayout::SetPlatformBinding(const std::variant<HlslPipelineConstantBinding, GlslPipelineConstantBinding>& inPlatformBinding)
    {
        platformBinding = inPlatformBinding;
        return *this;
    }

    PipelineLayoutCreateInfo::PipelineLayoutCreateInfo()
    {
    }

    PipelineLayoutCreateInfo& PipelineLayoutCreateInfo::AddBindGroupLayout(const BindGroupLayout* inLayout)
    {
        bindGroupLayouts.emplace_back(inLayout);
        return *this;
    }

    PipelineLayoutCreateInfo& PipelineLayoutCreateInfo::AddPipelineConstantLayout(const PipelineConstantLayout& inLayout)
    {
        pipelineConstantLayouts.emplace_back(inLayout);
        return *this;
    }

    PipelineLayout::PipelineLayout(const PipelineLayoutCreateInfo&) {}

    PipelineLayout::~PipelineLayout() = default;
}

//
// Created by johnk on 2022/8/3.
//

#include <format>

#include <Render/MeshRenderData.h>
#include <Render/RenderCache.h>
#include <Render/Renderer.h>
#include <Render/SceneProxy/Primitive.h>
#include <Render/Shader.h>

namespace Render::Internal {
    const Common::LinearColor surfaceClearColor = { 0.1f, 0.1f, 0.12f, 1.0f };

    struct ALIGN_AS_GPU BasePassVsUniform {
        Common::FMat4x4 localToWorld;
        Common::FMat4x4 worldToClip;
    };

    struct ALIGN_AS_GPU BasePassPsUniform {
        Common::FVec4 baseColor;
    };

    struct BasePassDraw {
        size_t viewIndex;
        RasterPipeline* pipeline;
        RGBindGroupRef bindGroup;
        RGBufferViewRef vertexBufferView;
        RGBufferViewRef indexBufferView;
        uint32_t indexCount;
    };

    static RVertexState BuildVertexState(const VertexFactoryType& inVertexFactoryType)
    {
        RVertexBufferLayout layout(RHI::VertexStepMode::perVertex, MeshRenderData::vertexStride);
        for (const auto& input : inVertexFactoryType.GetVertexInputs()) {
            layout.AddAttribute(RVertexAttribute(RVertexBinding(input.name, 0), input.format, input.offset));
        }
        return RVertexState().AddVertexBufferLayout(layout);
    }
}

namespace Render {
    Renderer::Renderer(const Params& inParams)
        : device(inParams.device)
        , scene(inParams.scene)
        , surface(inParams.surface)
        , surfaceExtent(inParams.surfaceExtent)
        , views(inParams.views)
        , waitSemaphore(inParams.waitSemaphore)
        , signalSemaphore(inParams.signalSemaphore)
        , signalFence(inParams.signalFence)
    {
    }

    Renderer::~Renderer() = default;

    StandardRenderer::StandardRenderer(const Params& inParams)
        : Renderer(inParams)
        , rgBuilder(*device)
    {
    }

    StandardRenderer::~StandardRenderer() = default;

    void StandardRenderer::Render(float inDeltaTimeSeconds)
    {
        const RHI::PixelFormat colorFormat = surface->GetCreateInfo().format;

        auto* backTexture = rgBuilder.ImportTexture(surface, RHI::TextureState::present);
        auto* backTextureView = rgBuilder.CreateTextureView(backTexture, RGTextureViewDesc(RHI::TextureViewType::colorAttachment, RHI::TextureViewDimension::tv2D));
        auto* depthTexture = rgBuilder.CreateTexture(
            RGTextureDesc()
                .SetDimension(RHI::TextureDimension::t2D)
                .SetWidth(surfaceExtent.x)
                .SetHeight(surfaceExtent.y)
                .SetDepthOrArraySize(1)
                .SetFormat(RHI::PixelFormat::d32Float)
                .SetUsages(RHI::TextureUsageBits::depthStencilAttachment)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(RHI::TextureState::depthStencilWrite)
                .SetDebugName("sceneDepth"));
        auto* depthTextureView = rgBuilder.CreateTextureView(depthTexture, RGTextureViewDesc(RHI::TextureViewType::depthStencil, RHI::TextureViewDimension::tv2D));

        std::vector<Internal::BasePassDraw> draws;
        std::vector<RGBindGroupRef> passBindGroups;
        if (scene != nullptr) {
            ShaderMap& shaderMap = ShaderMap::Get(*device);
            size_t drawIndex = 0;

            for (const auto& [entity, proxy] : scene->All<PrimitiveSceneProxy>()) {
                if (!proxy.mesh.Valid() || proxy.vertexFactoryType == nullptr || proxy.vertexShaderType == nullptr || proxy.pixelShaderType == nullptr) {
                    continue;
                }
                // material shaders compile asynchronously, primitives simply do not draw until artifacts arrive
                if (!shaderMap.HasShaderInstance(*proxy.vertexShaderType, {}) || !shaderMap.HasShaderInstance(*proxy.pixelShaderType, {})) {
                    continue;
                }

                const ShaderInstance vertexShader = shaderMap.GetShaderInstance(*proxy.vertexShaderType, {});
                const ShaderInstance pixelShader = shaderMap.GetShaderInstance(*proxy.pixelShaderType, {});
                auto* pipeline = PipelineCache::Get(*device).GetOrCreate(
                    RasterPipelineStateDesc()
                        .SetVertexShader(vertexShader)
                        .SetPixelShader(pixelShader)
                        .SetVertexState(Internal::BuildVertexState(*proxy.vertexFactoryType))
                        .SetPrimitiveState(RPrimitiveState().SetCullMode(RHI::CullMode::none))
                        .SetDepthStencilState(
                            RDepthStencilState()
                                .SetDepthEnabled(true)
                                .SetFormat(RHI::PixelFormat::d32Float)
                                .SetDepthCompareFunc(RHI::CompareFunc::greaterEqual))
                        .SetFragmentState(RFragmentState().AddColorTarget(RHI::ColorTargetState(colorFormat, RHI::ColorWriteBits::all, false))));

                auto* vertexBuffer = rgBuilder.ImportBuffer(proxy.mesh->GetVertexBuffer(), RHI::BufferState::shaderReadOnly);
                auto* vertexBufferView = rgBuilder.CreateBufferView(
                    vertexBuffer, RGBufferViewDesc(RHI::BufferViewType::vertex, vertexBuffer->GetDesc().size, 0, RHI::VertexBufferViewInfo(MeshRenderData::vertexStride)));
                auto* indexBuffer = rgBuilder.ImportBuffer(proxy.mesh->GetIndexBuffer(), RHI::BufferState::shaderReadOnly);
                auto* indexBufferView = rgBuilder.CreateBufferView(
                    indexBuffer, RGBufferViewDesc(RHI::BufferViewType::index, indexBuffer->GetDesc().size, 0, RHI::IndexBufferViewInfo(RHI::IndexFormat::uint32)));

                for (size_t viewIndex = 0; viewIndex < views.size(); viewIndex++) {
                    const View& view = views[viewIndex];

                    Internal::BasePassVsUniform vsUniform {};
                    vsUniform.localToWorld = proxy.localToWorld;
                    vsUniform.worldToClip = view.data.projectionMatrix * view.data.viewMatrix;

                    Internal::BasePassPsUniform psUniform {};
                    psUniform.baseColor = proxy.baseColor;

                    auto* vsUniformBuffer = rgBuilder.CreateBuffer(
                        RGBufferDesc(sizeof(Internal::BasePassVsUniform), RHI::BufferUsageBits::uniform | RHI::BufferUsageBits::mapWrite, RHI::BufferState::staging, std::format("basePassVsUniform{}", drawIndex)));
                    auto* vsUniformBufferView = rgBuilder.CreateBufferView(vsUniformBuffer, RGBufferViewDesc(RHI::BufferViewType::uniformBinding, sizeof(Internal::BasePassVsUniform)));
                    rgBuilder.QueueBufferUpload(vsUniformBuffer, RGBufferUploadInfo(&vsUniform, sizeof(Internal::BasePassVsUniform), 0, 0, true));

                    auto* psUniformBuffer = rgBuilder.CreateBuffer(
                        RGBufferDesc(sizeof(Internal::BasePassPsUniform), RHI::BufferUsageBits::uniform | RHI::BufferUsageBits::mapWrite, RHI::BufferState::staging, std::format("basePassPsUniform{}", drawIndex)));
                    auto* psUniformBufferView = rgBuilder.CreateBufferView(psUniformBuffer, RGBufferViewDesc(RHI::BufferViewType::uniformBinding, sizeof(Internal::BasePassPsUniform)));
                    rgBuilder.QueueBufferUpload(psUniformBuffer, RGBufferUploadInfo(&psUniform, sizeof(Internal::BasePassPsUniform), 0, 0, true));

                    auto* bindGroup = rgBuilder.AllocateBindGroup(
                        RGBindGroupDesc::Create(pipeline->GetPipelineLayout()->GetBindGroupLayout(0))
                            .UniformBuffer("vsUniform", vsUniformBufferView)
                            .UniformBuffer("materialUniform", psUniformBufferView));

                    Internal::BasePassDraw draw {};
                    draw.viewIndex = viewIndex;
                    draw.pipeline = pipeline;
                    draw.bindGroup = bindGroup;
                    draw.vertexBufferView = vertexBufferView;
                    draw.indexBufferView = indexBufferView;
                    draw.indexCount = proxy.mesh->GetIndexCount();
                    draws.emplace_back(draw);
                    passBindGroups.emplace_back(bindGroup);
                    drawIndex++;
                }
            }
        }

        rgBuilder.AddRasterPass(
            "BasePass",
            RGRasterPassDesc()
                .AddColorAttachment(RGColorAttachment(backTextureView, RHI::LoadOp::clear, RHI::StoreOp::store, Internal::surfaceClearColor))
                .SetDepthStencilAttachment(RGDepthStencilAttachment(depthTextureView, false, RHI::LoadOp::clear, RHI::StoreOp::discard, 0.0f)),
            passBindGroups,
            [draws = std::move(draws), views = views](const RGBuilder& rg, RHI::RasterPassCommandRecorder& recorder) -> void {
                for (size_t viewIndex = 0; viewIndex < views.size(); viewIndex++) {
                    const auto& viewport = views[viewIndex].data.viewport;
                    recorder.SetViewport(
                        static_cast<float>(viewport.min.x), static_cast<float>(viewport.min.y),
                        static_cast<float>(viewport.ExtentX()), static_cast<float>(viewport.ExtentY()), 0.0f, 1.0f);
                    recorder.SetScissor(viewport.min.x, viewport.min.y, viewport.ExtentX(), viewport.ExtentY());
                    recorder.SetPrimitiveTopology(RHI::PrimitiveTopology::triangleList);

                    for (const auto& draw : draws) {
                        if (draw.viewIndex != viewIndex) {
                            continue;
                        }
                        recorder.SetPipeline(draw.pipeline->GetRHI());
                        recorder.SetBindGroup(0, rg.GetRHI(draw.bindGroup));
                        recorder.SetVertexBuffer(0, rg.GetRHI(draw.vertexBufferView));
                        recorder.SetIndexBuffer(rg.GetRHI(draw.indexBufferView));
                        recorder.DrawIndexed(draw.indexCount, 1, 0, 0, 0);
                    }
                }
            },
            {},
            [backTexture](const RGBuilder& rg, RHI::CommandRecorder& recorder) -> void {
                recorder.ResourceBarrier(RHI::Barrier::Transition(rg.GetRHI(backTexture), RHI::TextureState::renderTarget, RHI::TextureState::present));
            });

        RGExecuteInfo executeInfo;
        executeInfo.semaphoresToWait = { waitSemaphore };
        executeInfo.semaphoresToSignal = { signalSemaphore };
        executeInfo.inFenceToSignal = signalFence;
        rgBuilder.Execute(executeInfo);

        FinalizeViews();
    }

    void StandardRenderer::FinalizeViews() const
    {
        for (const auto& view : views) {
            view.state->prevData = view.data;
        }
    }
}

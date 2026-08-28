//
// Created by johnk on 2024/6/20.
//

#include <Application.h>
#include <RenderTarget.h>
#include <RHI/RHI.h>
#include <Render/ShaderCompiler.h>
#include <Render/RenderGraph.h>
#include <Render/RenderThread.h>
#include <Core/Log.h>

using namespace Common;
using namespace Render;
using namespace RHI;

struct Vertex {
    FVec3 position;
};

class TriangleVS final : public StaticShaderType<TriangleVS> {
    ShaderTypeInfo(
        TriangleVS,
        RHI::ShaderStageBits::sVertex,
        "Engine/Test/Sample/Rendering-Triangle/Triangle.esl",
        "VSMain")

    EmptyIncludeDirectories
    EmptyVariantFieldVec
};

class TrianglePS final : public StaticShaderType<TrianglePS> {
public:
    ShaderTypeInfo(
        TrianglePS,
        RHI::ShaderStageBits::sPixel,
        "Engine/Test/Sample/Rendering-Triangle/Triangle.esl",
        "PSMain")

    EmptyIncludeDirectories
    EmptyVariantFieldVec
};

ImplementStaticShaderType(TriangleVS);
ImplementStaticShaderType(TrianglePS);

struct PsUniform {
    FVec3 pixelColor;
};

class TriangleApplication final : public Application {
public:
    explicit TriangleApplication(const std::string& inName);
    ~TriangleApplication() override;

    void OnCreate() override;
    void OnDrawFrame() override;
    void OnDestroy() override;

private:
    void CreateDevice();
    void CompileAllShaders() const;
    void FetchShaderInstances();
    void CreateTriangleVertexBuffer();

    ShaderInstance triangleVS;
    ShaderInstance trianglePS;
    UniquePtr<Device> device;
    UniquePtr<SampleRenderTarget> renderTarget;
    UniquePtr<Buffer> triangleVertexBuffer;
};

TriangleApplication::TriangleApplication(const std::string& inName)
    : Application(inName)
{
}

TriangleApplication::~TriangleApplication() = default;

void TriangleApplication::OnCreate()
{
    CompileAllShaders();
    RenderThread::Get().Start();
    RenderWorkerThreads::Get().Start();

    CreateDevice();
    renderTarget = CreateRenderTarget(*device);

    RenderThread::Get().EmplaceTask([this]() -> void {
        FetchShaderInstances();
        renderTarget->Initialize();
        CreateTriangleVertexBuffer();
    });
}

void TriangleApplication::OnDrawFrame()
{
    RenderThread::Get().EmplaceTask([this]() -> void {
        const auto frame = renderTarget->Acquire();

        auto* pso = Render::PipelineCache::Get(*device).GetOrCreate(
            RasterPipelineStateDesc()
                .SetVertexShader(triangleVS)
                .SetPixelShader(trianglePS)
                .SetVertexState(
                    RVertexState()
                        .AddVertexBufferLayout(
                            RVertexBufferLayout(VertexStepMode::perVertex, sizeof(Vertex))
                                .AddAttribute(RVertexAttribute(RVertexBinding("POSITION", 0), VertexFormat::float32X3, offsetof(Vertex, position)))))
                .SetFragmentState(
                    RFragmentState()
                        .AddColorTarget(ColorTargetState(renderTarget->GetFormat(), ColorWriteBits::all, false))));

        RGBuilder builder(*device);
        auto* backTexture = builder.ImportTexture(frame.texture, frame.initialState);
        auto* backTextureView = builder.CreateTextureView(backTexture, RGTextureViewDesc(TextureViewType::colorAttachment, TextureViewDimension::tv2D));
        auto* vertexBuffer = builder.ImportBuffer(triangleVertexBuffer.Get(), BufferState::shaderReadOnly);
        auto* vertexBufferView = builder.CreateBufferView(vertexBuffer, RGBufferViewDesc(BufferViewType::vertex, vertexBuffer->GetDesc().size, 0, VertexBufferViewInfo(sizeof(Vertex))));
        auto* psUniformBuffer = builder.CreateBuffer(RGBufferDesc(sizeof(PsUniform), BufferUsageBits::uniform | BufferUsageBits::mapWrite, BufferState::staging, "psUniform"));
        auto* psUniformBufferView = builder.CreateBufferView(psUniformBuffer, RGBufferViewDesc(BufferViewType::uniformBinding, sizeof(PsUniform)));

        auto* bindGroup = builder.AllocateBindGroup(
            RGBindGroupDesc::Create(pso->GetPipelineLayout()->GetBindGroupLayout(0))
                .UniformBuffer("psUniform", psUniformBufferView));

        PsUniform psUniform {};
        psUniform.pixelColor = FVec3(
            (std::sin(GetCurrentTimeSeconds()) + 1) / 2,
            (std::cos(GetCurrentTimeSeconds()) + 1) / 2,
            std::abs(std::sin(GetCurrentTimeSeconds())));

        builder.QueueBufferUpload(
            psUniformBuffer,
            RGBufferUploadInfo(&psUniform, sizeof(PsUniform)));

        builder.AddRasterPass(
            "BasePass",
            RGRasterPassDesc()
                .AddColorAttachment(RGColorAttachment(backTextureView, LoadOp::clear, StoreOp::store)),
            { bindGroup },
            [pso, vertexBufferView, bindGroup, viewportWidth = GetWindowWidth(), viewportHeight = GetWindowHeight()](const RGBuilder& rg, RasterPassCommandRecorder& recorder) -> void {
                recorder.SetPipeline(pso->GetRHI());
                recorder.SetScissor(0, 0, viewportWidth, viewportHeight);
                recorder.SetViewport(0, 0, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0, 1);
                recorder.SetVertexBuffer(0, rg.GetRHI(vertexBufferView));
                recorder.SetPrimitiveTopology(PrimitiveTopology::triangleList);
                recorder.SetBindGroup(0, rg.GetRHI(bindGroup));
                recorder.Draw(3, 1, 0, 0);
            },
            {},
            [this, backTexture](const RGBuilder& rg, CommandRecorder& recorder) -> void {
                renderTarget->FinishRenderPass(rg, recorder, backTexture);
            });

        renderTarget->PrepareForSubmit(builder, backTexture);
        renderTarget->Execute(builder, frame);

        Core::ThreadContext::IncFrameNumber();
        BufferPool::Get(*device).Forfeit();
        TexturePool::Get(*device).Forfeit();
        ResourceViewCache::Get(*device).Forfeit();
        BindGroupCache::Get(*device).Forfeit();
    });

    // TODO in sample, just sync with render thread every frame, maybe later need a better render-thread based application class
    RenderThread::Get().Flush();
}

void TriangleApplication::OnDestroy()
{
    RenderThread::Get().EmplaceTask([this]() -> void {
        const UniquePtr<Fence> fence = device->CreateFence(false);
        device->GetQueue(QueueType::graphics, 0)->Flush(fence.Get());
        fence->Wait();

        renderTarget = nullptr;
        DestroyDeviceResources(*device);
    });
    RenderThread::Get().Flush();

    RenderWorkerThreads::Get().Stop();
    RenderThread::Get().Stop();
}

void TriangleApplication::CreateDevice()
{
    device = GetGpu()
        ->RequestDevice(
            DeviceCreateInfo()
                .AddQueueRequest(QueueRequestInfo(QueueType::graphics, 1)));
}

void TriangleApplication::CompileAllShaders() const
{
    ShaderCompileOptions options;
    options.includeDirectories = {"../Test/Sample/ShaderInclude", "../Test/Sample/Rendering-Triangle"};
    options.byteCodeType = GetRHIType() == RHI::RHIType::directX12 ? ShaderByteCodeType::dxil : ShaderByteCodeType::spirv;
    options.withDebugInfo = false;
    auto result = ShaderTypeCompiler::Get().CompileAll(options);
    const auto& [success, errorInfo] = result.get();
    Assert(success);
}

void TriangleApplication::FetchShaderInstances()
{
    ShaderArtifactRegistry::Get().PerformThreadCopy();
    triangleVS = ShaderMap::Get(*device).GetShaderInstance(TriangleVS::Get(), {});
    trianglePS = ShaderMap::Get(*device).GetShaderInstance(TrianglePS::Get(), {});
}

void TriangleApplication::CreateTriangleVertexBuffer()
{
    const std::vector<Vertex> vertices = {
        { { -.5f, -.5f, 0.f } },
        { { .5f, -.5f, 0.f } },
        { { 0.f, .5f, 0.f } },
    };

    const BufferCreateInfo bufferCreateInfo = BufferCreateInfo()
        .SetSize(vertices.size() * sizeof(Vertex))
        .SetUsages(BufferUsageBits::vertex | BufferUsageBits::mapWrite | BufferUsageBits::copySrc)
        .SetInitialState(BufferState::staging)
        .SetDebugName("vertexBuffer");

    triangleVertexBuffer = device->CreateBuffer(bufferCreateInfo);
    if (triangleVertexBuffer != nullptr) {
        auto* data = triangleVertexBuffer->Map(MapMode::write, 0, bufferCreateInfo.size);
        memcpy(data, vertices.data(), bufferCreateInfo.size);
        triangleVertexBuffer->Unmap();
    }
}

int main(int argc, char* argv[])
{
    TriangleApplication application("Rendering-Triangle");
    if (!application.Initialize(argc, argv)) {
        return -1;
    }
    return application.RunLoop();
}

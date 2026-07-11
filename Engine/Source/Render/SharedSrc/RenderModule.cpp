//
// Created by johnk on 2023/8/4.
//

#include <Core/Thread.h>
#include <Render/RenderCache.h>
#include <Render/RenderModule.h>
#include <Render/ResourcePool.h>
#include <Render/Scene.h>

namespace Render {
    RenderModule::RenderModule()
        : initialized(false)
        , rhiInstance(nullptr)
    {
    }

    RenderModule::~RenderModule() = default;

    void RenderModule::OnLoad()
    {
        Module::OnLoad();
    }

    void RenderModule::OnUnload()
    {
        Module::OnUnload();
    }

    Core::ModuleType RenderModule::Type() const
    {
        return Core::ModuleType::mDynamic;
    }

    void RenderModule::Initialize(const RenderModuleInitParams& inParams)
    {
        Assert(!initialized);

        RenderThread::Get().Start();
        RenderWorkerThreads::Get().Start();

        rhiInstance = RHI::Instance::GetByType(inParams.rhiType);
        rhiDevice = rhiInstance->GetGpu(0)->RequestDevice(
            RHI::DeviceCreateInfo()
                .AddQueueRequest(RHI::QueueRequestInfo(RHI::QueueType::graphics, 1))
                .AddQueueRequest(RHI::QueueRequestInfo(RHI::QueueType::compute, 1))
                .AddQueueRequest(RHI::QueueRequestInfo(RHI::QueueType::transfer, 1)));

        initialized = true;
    }

    void RenderModule::DeInitialize()
    {
        RenderThread::Get().Stop();
        RenderWorkerThreads::Get().Stop();

        DestroyDeviceResources(*rhiDevice);

        rhiInstance = nullptr;
        rhiDevice = nullptr;
        initialized = false;
    }

    RHI::Device* RenderModule::GetDevice() const
    {
        return rhiDevice.Get();
    }

    Render::RenderThread& RenderModule::GetRenderThread() const // NOLINT
    {
        return RenderThread::Get();
    }

    void RenderModule::BeginFrame() const // NOLINT
    {
        ShaderArtifactRegistry::Get().PerformThreadCopy();
        BufferPool::Get(*rhiDevice).Forfeit();
        TexturePool::Get(*rhiDevice).Forfeit();
        ResourceViewCache::Get(*rhiDevice).Forfeit();
        BindGroupCache::Get(*rhiDevice).Forfeit();
    }

    Scene* RenderModule::NewScene() const // NOLINT
    {
        return new Scene();
    }

    ViewState* RenderModule::NewViewState() const // NOLINT
    {
        return new ViewState();
    }

    View RenderModule::CreateView() const // NOLINT
    {
        return View();
    }

    std::future<ShaderTypeCompileResult> RenderModule::CompileShaderTypes(const std::vector<const ShaderType*>& inShaderTypes, const ShaderCompileOptions& inOptions) const
    {
        return ShaderTypeCompiler::Get().Compile(inShaderTypes, inOptions);
    }

    Common::UniquePtr<Renderer> RenderModule::CreateStandardRenderer(const StandardRenderer::Params& inParams) const // NOLINT
    {
        return Common::UniquePtr<Renderer>(new StandardRenderer(inParams));
    }
} // namespace Render

IMPLEMENT_DYNAMIC_MODULE(RENDER_API, Render::RenderModule);

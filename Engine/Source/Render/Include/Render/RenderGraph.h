//
// Created by johnk on 2023/11/28.
//

#pragma once

#include <unordered_map>
#include <functional>
#include <future>
#include <optional>
#include <variant>

#include <Common/Memory.h>
#include <RHI/RHI.h>
#include <Render/ResourcePool.h>
#include <Render/RenderCache.h>

namespace Render {
    class RGBuilder;

    enum class RGResType : uint8_t {
        buffer,
        texture,
        max
    };

    enum class RGResViewType : uint8_t {
        bufferView,
        textureView,
        max
    };

    enum class RGPassType : uint8_t {
        copy,
        compute,
        raster,
        max
    };

    enum class RGQueueType : uint8_t {
        main,
        asyncCompute,
        asyncCopy,
        max
    };

    using RGBufferDesc = RHI::BufferCreateInfo;
    using RGTextureDesc = RHI::TextureCreateInfo;

    class RGResource {
    public:
        virtual ~RGResource();

        RGResType Type() const;
        void MaskAsUsed();

    protected:
        friend class RGBuilder;
        friend class RGPass;

        explicit RGResource(RGResType inType);

        RGResType type;
        bool forceUsed;
        bool imported;
    };

    class RGBuffer final : public RGResource {
    public:
        ~RGBuffer() override;

        const RGBufferDesc& GetDesc() const;

    private:
        friend class RGBuilder;
        friend class RGPass;

        explicit RGBuffer(RGBufferDesc inDesc);
        explicit RGBuffer(RHI::Buffer* inImportedBuffer, RHI::BufferState inInitialState);

        RGBufferDesc desc;
        RHI::Buffer* rhiHandleImported;
    };

    class RGTexture final : public RGResource {
    public:
        ~RGTexture() override;

        const RGTextureDesc& GetDesc() const;

    private:
        friend class RGBuilder;
        friend class RGPass;

        explicit RGTexture(RGTextureDesc inDesc);
        explicit RGTexture(RHI::Texture* inImportedTexture, RHI::TextureState inInitialState);

        RGTextureDesc desc;
        RHI::Texture* rhiHandleImported;
    };

    using RGResourceRef = RGResource*;
    using RGBufferRef = RGBuffer*;
    using RGTextureRef = RGTexture*;

    using RGBufferViewDesc = RHI::BufferViewCreateInfo;
    using RGTextureViewDesc = RHI::TextureViewCreateInfo;

    class RGResourceView {
    public:
        virtual ~RGResourceView();

        RGResViewType Type() const;

        virtual RGResourceRef GetResource() = 0;

    protected:
        explicit RGResourceView(RGResViewType inType);

        RGResViewType type;
    };

    class RGBufferView final : public RGResourceView {
    public:
        ~RGBufferView() override;

        const RGBufferViewDesc& GetDesc() const;
        RGBufferRef GetBuffer() const;
        RGResourceRef GetResource() override;

    private:
        friend class RGBuilder;

        RGBufferView(RGBufferRef inBuffer, const RGBufferViewDesc& inDesc);

        RGBufferRef buffer;
        RGBufferViewDesc desc;
    };

    class RGTextureView final : public RGResourceView {
    public:
        ~RGTextureView() override;

        const RGTextureViewDesc& GetDesc() const;
        RGTextureRef GetTexture() const;

        RGResourceRef GetResource() override;

    private:
        friend class RGBuilder;

        RGTextureView(RGTextureRef inTexture, const RGTextureViewDesc& inDesc);

        RGTextureRef texture;
        RGTextureViewDesc desc;
    };

    using RGResourceViewRef = RGResourceView*;
    using RGBufferViewRef = RGBufferView*;
    using RGTextureViewRef = RGTextureView*;

    struct RGColorAttachment : RHI::ColorAttachmentBase<RGColorAttachment> {
        RGTextureViewRef view;
        // TODO TextureView* resolve

        explicit RGColorAttachment(
            RGTextureViewRef inView = nullptr,
            RHI::LoadOp inLoadOp = RHI::LoadOp::load,
            RHI::StoreOp inStoreOp = RHI::StoreOp::discard,
            const Common::LinearColor& inClearValue = Common::LinearColorConsts::black);

        RGColorAttachment& SetView(RGTextureViewRef inView);
    };

    struct RGDepthStencilAttachment : RHI::DepthStencilAttachmentBase<RGDepthStencilAttachment> {
        RGTextureViewRef view;

        explicit RGDepthStencilAttachment(
            RGTextureViewRef inView = nullptr,
            bool inDepthReadOnly = true,
            RHI::LoadOp inDepthLoadOp = RHI::LoadOp::load,
            RHI::StoreOp inDepthStoreOp = RHI::StoreOp::discard,
            float inDepthClearValue = 0.0f,
            bool inStencilReadOnly = true,
            RHI::LoadOp inStencilLoadOp = RHI::LoadOp::load,
            RHI::StoreOp inStencilStoreOp = RHI::StoreOp::discard,
            uint32_t inStencilClearValue = 0);

        RGDepthStencilAttachment& SetView(RGTextureViewRef inView);
    };

    struct RGRasterPassDesc {
        std::vector<RGColorAttachment> colorAttachments;
        std::optional<RGDepthStencilAttachment> depthStencilAttachment;

        RGRasterPassDesc& AddColorAttachment(const RGColorAttachment& inAttachment);
        RGRasterPassDesc& SetDepthStencilAttachment(const RGDepthStencilAttachment& inAttachment);
    };

    struct RGCopyPassDesc {
        std::vector<RGResourceRef> copySrcs;
        std::vector<RGResourceRef> copyDsts;
    };

    struct RGBindItemDesc {
        RHI::BindingType type;
        std::variant<RGBufferViewRef, RGTextureViewRef, RHI::Sampler*> view;
    };

    struct RGBindGroupDesc {
        BindGroupLayout* layout;
        std::unordered_map<std::string, RGBindItemDesc> items;

        static RGBindGroupDesc Create(BindGroupLayout* inLayout);
        RGBindGroupDesc& Sampler(std::string inName, RHI::Sampler* inSampler);
        RGBindGroupDesc& UniformBuffer(std::string inName, RGBufferViewRef bufferView);
        RGBindGroupDesc& StorageBuffer(std::string inName, RGBufferViewRef bufferView);
        RGBindGroupDesc& RwStorageBuffer(std::string inName, RGBufferViewRef bufferView);
        RGBindGroupDesc& Texture(std::string inName, RGTextureViewRef textureView);
        RGBindGroupDesc& StorageTexture(std::string inName, RGTextureViewRef textureView);
    };

    class RGBindGroup {
    public:
        ~RGBindGroup();

        const RGBindGroupDesc& GetDesc() const;

    private:
        friend class RGBuilder;

        explicit RGBindGroup(RGBindGroupDesc inDesc);

        RGBindGroupDesc desc;
    };

    using RGBindGroupRef = RGBindGroup*;

    struct RGBufferUploadInfo {
        struct DataView {
            void* data;
            size_t size;
        };

        struct DataCopy {
            std::vector<uint8_t> data;
        };

        std::variant<std::monostate, DataView, DataCopy> src;
        size_t srcOffset;
        size_t dstOffset;

        RGBufferUploadInfo();
        RGBufferUploadInfo(void* inData, size_t inSize, size_t inSrcOffset = 0, size_t inDstOffset = 0, bool inCopy = false);
    };

    class RGPass {
    public:
        virtual ~RGPass();

    protected:
        friend class RGBuilder;

        RGPass(std::string inName, RGPassType inType);

        std::string name;
        RGPassType type;
    };

    using RGPassRef = RGPass*;

    using RGCopyPassExecuteFunc = std::function<void(const RGBuilder&, RHI::CopyPassCommandRecorder&)>;
    using RGComputePassExecuteFunc = std::function<void(const RGBuilder&, RHI::ComputePassCommandRecorder&)>;
    using RGRasterPassExecuteFunc = std::function<void(const RGBuilder&, RHI::RasterPassCommandRecorder&)>;
    using RGCommonPassExecuteFunc = std::function<void(const RGBuilder&, RHI::CommandRecorder&)>;

    class RGCopyPass final : public RGPass {
    public:
        ~RGCopyPass() override;

    private:
        friend class RGBuilder;

        RGCopyPass(
            std::string inName,
            RGCopyPassDesc inPassDesc,
            RGCopyPassExecuteFunc inFunc,
            RGCommonPassExecuteFunc inPreExecuteFunc = {},
            RGCommonPassExecuteFunc inPostExecuteFunc = {});

        RGCopyPassDesc passDesc;
        RGCopyPassExecuteFunc passFunc;
        RGCommonPassExecuteFunc prePassFunc;
        RGCommonPassExecuteFunc postPassFunc;
    };

    class RGComputePass final : public RGPass {
    public:
        ~RGComputePass() override;

    private:
        friend class RGBuilder;

        RGComputePass(
            std::string inName,
            std::vector<RGBindGroupRef> inBindGroups,
            RGComputePassExecuteFunc inFunc,
            RGCommonPassExecuteFunc inPreExecuteFunc = {},
            RGCommonPassExecuteFunc inPostExecuteFunc = {});

        RGComputePassExecuteFunc passFunc;
        RGCommonPassExecuteFunc prePassFunc;
        RGCommonPassExecuteFunc postPassFunc;
        std::vector<RGBindGroupRef> bindGroups;
    };

    class RGRasterPass final : public RGPass {
    public:
        ~RGRasterPass() override;

    private:
        friend class RGBuilder;

        RGRasterPass(
            std::string inName, RGRasterPassDesc inPassDesc,
            std::vector<RGBindGroupRef> inBindGroups,
            RGRasterPassExecuteFunc inFunc,
            RGCommonPassExecuteFunc inPreExecuteFunc = {},
            RGCommonPassExecuteFunc inPostExecuteFunc = {});

        RGRasterPassDesc passDesc;
        RGRasterPassExecuteFunc passFunc;
        RGCommonPassExecuteFunc prePassFunc;
        RGCommonPassExecuteFunc postPassFunc;
        std::vector<RGBindGroupRef> bindGroups;
    };

    struct RGExecuteInfo {
        std::vector<RHI::Semaphore*> semaphoresToWait;
        std::vector<RHI::Semaphore*> semaphoresToSignal;
        RHI::Fence* inFenceToSignal = nullptr;
    };

    class RGBuilder {
    public:
        NonCopyable(RGBuilder);
        NonMovable(RGBuilder);
        explicit RGBuilder(RHI::Device& inDevice);
        ~RGBuilder();

        // setup
        RGBufferRef CreateBuffer(const RGBufferDesc& inDesc);
        RGTextureRef CreateTexture(const RGTextureDesc& inDesc);
        RGBufferViewRef CreateBufferView(RGBufferRef inBuffer, const RGBufferViewDesc& inDesc);
        RGTextureViewRef CreateTextureView(RGTextureRef inTexture, const RGTextureViewDesc& inDesc);
        RGBufferRef ImportBuffer(RHI::Buffer* inBuffer, RHI::BufferState inInitialState);
        RGTextureRef ImportTexture(RHI::Texture* inTexture, RHI::TextureState inInitialState);
        RGBindGroupRef AllocateBindGroup(const RGBindGroupDesc& inDesc);
        void QueueBufferUpload(RGBufferRef inBuffer, const RGBufferUploadInfo& inUploadInfo);
        void AddCopyPass(const std::string& inName, const RGCopyPassDesc& inPassDesc, const RGCopyPassExecuteFunc& inFunc, bool inAsyncCopy = false, const RGCommonPassExecuteFunc& inPreExecuteFunc = {}, const RGCommonPassExecuteFunc& inPostExecuteFunc = {});
        void AddComputePass(const std::string& inName, const std::vector<RGBindGroupRef>& inBindGroups, const RGComputePassExecuteFunc& inFunc, bool inAsyncCompute = false, const RGCommonPassExecuteFunc& inPreExecuteFunc = {}, const RGCommonPassExecuteFunc& inPostExecuteFunc = {});
        void AddRasterPass(const std::string& inName, const RGRasterPassDesc& inPassDesc, const std::vector<RGBindGroupRef>& inBindGroups, const RGRasterPassExecuteFunc& inFunc, const RGCommonPassExecuteFunc& inPreExecuteFunc = {}, const RGCommonPassExecuteFunc& inPostExecuteFunc = {});
        void AddSyncPoint();

        // execute
        void Execute(const RGExecuteInfo& inExecuteInfo);
        RHI::Buffer* GetRHI(RGBufferRef inBuffer) const;
        RHI::Texture* GetRHI(RGTextureRef inTexture) const;
        RHI::BufferView* GetRHI(RGBufferViewRef inBufferView) const;
        RHI::TextureView* GetRHI(RGTextureViewRef inTextureView) const;
        RHI::BindGroup* GetRHI(RGBindGroupRef inBindGroup) const;

    private:
        struct AsyncTimelineExecuteContext {
            std::unordered_map<RGQueueType, Common::UniquePtr<RHI::CommandBuffer>> queueCmdBufferMap;
            std::unordered_map<RGQueueType, Common::UniquePtr<RHI::Semaphore>> queueSemaphoreToSignalMap;

            AsyncTimelineExecuteContext();
            AsyncTimelineExecuteContext(AsyncTimelineExecuteContext&& inOther) noexcept;
        };

        void Compile();
        void ExecuteInternal(const RGExecuteInfo& inExecuteInfo);

        void CompilePassReadWrites();
        void PerformSyncCheck() const;
        void PerformCull();
        // TODO resource states check inside pass (e.g. read/write a resource within a pass)
        void ComputeResourcesInitialState();
        void ExecuteCopyPass(RHI::CommandRecorder& inRecoder, RGCopyPass* inCopyPass);
        void ExecuteComputePass(RHI::CommandRecorder& inRecoder, RGComputePass* inComputePass);
        void ExecuteRasterPass(RHI::CommandRecorder& inRecoder, RGRasterPass* inRasterPass);
        void PerformBufferUploads();
        void WaitBufferUploadsFinish() const;
        void DevirtualizeViewsCreatedOnImportedResources();
        void DevirtualizeResource(RGResourceRef inResource);
        void DevirtualizeResources(const std::unordered_set<RGResourceRef>& inResources);
        void DevirtualizeBindGroupsAndViews(const std::vector<RGBindGroupRef>& inBindGroups);
        void DevirtualizeAttachmentViews(const RGRasterPassDesc& inDesc);
        void FinalizePassResources(const std::unordered_set<RGResourceRef>& inResources);
        void FinalizePassBindGroups(const std::vector<RGBindGroupRef>& inBindGroups);
        void TransitionResourcesForCopyPassDesc(RHI::CommandCommandRecorder& inRecoder, const RGCopyPassDesc& inDesc);
        void TransitionResourcesForRasterPassDesc(RHI::CommandCommandRecorder& inRecoder, const RGRasterPassDesc& inDesc);
        void TransitionResourcesForBindGroups(RHI::CommandCommandRecorder& inRecoder, const std::vector<RGBindGroupRef>& inBindGroups);
        void TransitionBuffer(RHI::CommandCommandRecorder& inRecoder, RGBufferRef inBuffer, RHI::BufferState inState);
        void TransitionTexture(RHI::CommandCommandRecorder& inRecoder, RGTextureRef inTexture, RHI::TextureState inState);

        bool executed;
        RHI::Device& device;
        std::vector<Common::UniquePtr<RGResource>> resources;
        std::vector<Common::UniquePtr<RGResourceView>> views;
        std::vector<Common::UniquePtr<RGBindGroup>> bindGroups;
        std::vector<Common::UniquePtr<RGPass>> passes;
        std::unordered_map<RGQueueType, std::vector<RGPassRef>> recordingAsyncTimeline;
        std::vector<std::unordered_map<RGQueueType, std::vector<RGPassRef>>> asyncTimelines;
        std::unordered_map<RGBufferRef, RGBufferUploadInfo> bufferUploads;

        // execute context
        std::unordered_map<RGResourceRef, uint32_t> resourceReadCounts;
        std::unordered_map<RGPassRef, std::unordered_set<RGResourceRef>> passReadsMap;
        std::unordered_map<RGPassRef, std::unordered_set<RGResourceRef>> passWritesMap;
        std::unordered_set<RGResourceRef> culledResources;
        std::unordered_set<RGPassRef> culledPasses;
        std::unordered_map<RGResourceRef, std::variant<RHI::BufferState, RHI::TextureState>> resourceStates;
        std::vector<AsyncTimelineExecuteContext> asyncTimelineExecuteContexts;
        std::unordered_map<RGResourceRef, std::variant<PooledBufferRef, PooledTextureRef>> devirtualizedResources;
        std::unordered_map<RGResourceViewRef, std::variant<RHI::BufferView*, RHI::TextureView*>> devirtualizedResourceViews;
        std::unordered_map<RGBindGroupRef, RHI::BindGroup*> devirtualizedBindGroups;
        std::vector<std::future<void>> bufferUploadTasks;
    };
}

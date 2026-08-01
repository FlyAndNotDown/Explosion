#include <array>
#include <cstring>

#include <Test/Test.h>

#include <Render/RenderCache.h>
#include <Render/RenderGraph.h>
#include <Render/RenderThread.h>

namespace Render {
    struct RenderGraphTest : testing::Test {
        void SetUp() override
        {
            instance = RHI::Instance::GetByType(RHI::RHIType::dummy);
            device = instance->GetGpu(0)->RequestDevice(RHI::DeviceCreateInfo().AddQueueRequest(RHI::QueueRequestInfo(RHI::QueueType::graphics, 1)));
            RenderWorkerThreads::Get().Start();
        }

        void TearDown() override
        {
            RenderWorkerThreads::Get().Stop();
            DestroyDeviceResources(*device);
        }

        RHI::Instance* instance;
        Common::UniquePtr<RHI::Device> device;
    };

    TEST_F(RenderGraphTest, SkipsUploadsForCulledBuffers)
    {
        RGBuilder builder(*device);
        auto* buffer = builder.CreateBuffer(RGBufferDesc(4, RHI::BufferUsageBits::mapWrite, RHI::BufferState::staging));
        const std::array<uint8_t, 4> source = { 1, 2, 3, 4 };
        builder.QueueBufferUpload(buffer, RGBufferUploadInfo(source.data(), source.size()));

        builder.Execute({});

        ASSERT_EQ(BufferPool::Get(*device).Size(), 0);
    }

    TEST_F(RenderGraphTest, AppliesUploadsInOrderAndOwnsSourceDataByDefault)
    {
        RGBuilder builder(*device);
        auto* buffer = builder.CreateBuffer(RGBufferDesc(4, RHI::BufferUsageBits::mapRead | RHI::BufferUsageBits::mapWrite, RHI::BufferState::staging));
        buffer->MaskAsUsed();

        std::array<uint8_t, 4> initialSource = { 1, 2, 3, 4 };
        builder.QueueBufferUpload(buffer, RGBufferUploadInfo(initialSource.data(), initialSource.size()));
        initialSource.fill(0);

        const std::array<uint8_t, 3> offsetSource = { 8, 9, 10 };
        builder.QueueBufferUpload(buffer, RGBufferUploadInfo(offsetSource.data(), offsetSource.size(), 1, 1));

        const uint8_t finalByte = 7;
        builder.QueueBufferUpload(buffer, RGBufferUploadInfo(&finalByte, sizeof(finalByte), 0, 3));
        builder.Execute({});

        auto* const uploadedData = static_cast<const uint8_t*>(builder.GetRHI(buffer)->Map(RHI::MapMode::read, 0, 4));
        const std::array<uint8_t, 4> expected = { 1, 9, 10, 7 };
        ASSERT_EQ(std::memcmp(uploadedData, expected.data(), expected.size()), 0);
        builder.GetRHI(buffer)->Unmap();
    }

    TEST_F(RenderGraphTest, KeepsAllResourcesRequiredByLivePass)
    {
        RGBuilder builder(*device);
        auto* retainedBuffer = builder.CreateBuffer(RGBufferDesc(4, RHI::BufferUsageBits::copyDst, RHI::BufferState::copyDst));
        auto* siblingBuffer = builder.CreateBuffer(RGBufferDesc(4, RHI::BufferUsageBits::copyDst, RHI::BufferState::copyDst));
        retainedBuffer->MaskAsUsed();

        bool executed = false;
        RGCopyPassDesc passDesc;
        passDesc.copyDsts = { retainedBuffer, siblingBuffer };
        builder.AddCopyPass(
            "KeepAllResources",
            passDesc,
            [&executed, siblingBuffer](const RGBuilder& rg, RHI::CopyPassCommandRecorder&) -> void {
                ASSERT_NE(rg.GetRHI(siblingBuffer), nullptr);
                executed = true;
            });

        builder.Execute({});

        ASSERT_TRUE(executed);
    }

    TEST_F(RenderGraphTest, DoesNotKeepPassAliveThroughItsOwnLoad)
    {
        RGBuilder builder(*device);
        auto* texture = builder.CreateTexture(
            RGTextureDesc()
                .SetType(RHI::TextureType::t2D)
                .SetWidth(4)
                .SetHeight(4)
                .SetDepthOrArraySize(1)
                .SetFormat(RHI::PixelFormat::rgba8Unorm)
                .SetUsages(RHI::TextureUsageBits::renderAttachment)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(RHI::TextureState::renderTarget));
        auto* view = builder.CreateTextureView(
            texture,
            RGTextureViewDesc(RHI::TextureViewType::colorAttachment, RHI::TextureViewDimension::tv2D));

        bool executed = false;
        builder.AddRasterPass(
            "DeadLoad",
            RGRasterPassDesc().AddColorAttachment(RGColorAttachment(view, RHI::LoadOp::load, RHI::StoreOp::store)),
            {},
            [&executed](const RGBuilder&, RHI::RasterPassCommandRecorder&) -> void {
                executed = true;
            });

        builder.Execute({});

        ASSERT_FALSE(executed);
        ASSERT_EQ(TexturePool::Get(*device).Size(), 0);
    }

    TEST_F(RenderGraphTest, InfersReadOnlyDepthDependency)
    {
        RGBuilder builder(*device);
        auto* depthTexture = builder.CreateTexture(
            RGTextureDesc()
                .SetType(RHI::TextureType::t2D)
                .SetWidth(4)
                .SetHeight(4)
                .SetDepthOrArraySize(1)
                .SetFormat(RHI::PixelFormat::d32Float)
                .SetUsages(RHI::TextureUsageBits::copyDst | RHI::TextureUsageBits::depthStencilAttachment)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(RHI::TextureState::copyDst));
        auto* depthView = builder.CreateTextureView(
            depthTexture,
            RGTextureViewDesc(
                RHI::TextureViewType::depthStencil,
                RHI::TextureViewDimension::tv2D,
                RHI::TextureAspect::depth));
        auto* colorTexture = builder.CreateTexture(
            RGTextureDesc()
                .SetType(RHI::TextureType::t2D)
                .SetWidth(4)
                .SetHeight(4)
                .SetDepthOrArraySize(1)
                .SetFormat(RHI::PixelFormat::rgba8Unorm)
                .SetUsages(RHI::TextureUsageBits::renderAttachment)
                .SetMipLevels(1)
                .SetSamples(1)
                .SetInitialState(RHI::TextureState::renderTarget));
        auto* colorView = builder.CreateTextureView(
            colorTexture,
            RGTextureViewDesc(RHI::TextureViewType::colorAttachment, RHI::TextureViewDimension::tv2D));
        colorTexture->MaskAsUsed();

        bool producerExecuted = false;
        RGCopyPassDesc producerDesc;
        producerDesc.copyDsts = { depthTexture };
        builder.AddCopyPass(
            "DepthProducer",
            producerDesc,
            [&producerExecuted](const RGBuilder&, RHI::CopyPassCommandRecorder&) -> void {
                producerExecuted = true;
            });

        bool consumerExecuted = false;
        builder.AddRasterPass(
            "DepthConsumer",
            RGRasterPassDesc()
                .AddColorAttachment(RGColorAttachment(colorView, RHI::LoadOp::clear, RHI::StoreOp::store))
                .SetDepthStencilAttachment(RGDepthStencilAttachment(depthView, true, RHI::LoadOp::load, RHI::StoreOp::discard)),
            {},
            [&consumerExecuted](const RGBuilder&, RHI::RasterPassCommandRecorder&) -> void {
                consumerExecuted = true;
            });

        builder.Execute({});

        ASSERT_TRUE(producerExecuted);
        ASSERT_TRUE(consumerExecuted);
    }
}

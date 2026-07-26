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
}

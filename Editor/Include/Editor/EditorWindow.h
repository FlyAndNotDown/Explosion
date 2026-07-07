//
// Created by johnk on 2026/7/7.
//

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include <Common/Math/Color.h>
#include <Common/Math/Vector.h>
#include <Common/Memory.h>
#include <Render/ShaderCompiler.h>
#include <RHI/RHI.h>
#include <Runtime/Window.h>

struct GLFWwindow;
struct ImDrawData;

namespace Editor {
    struct EditorWindowDesc {
        std::string title;
        uint32_t width;
        uint32_t height;
    };

    class EditorWindow final : public Runtime::Window {
    public:
        explicit EditorWindow(const EditorWindowDesc& inDesc);
        ~EditorWindow() override;

        NonCopyable(EditorWindow)
        NonMovable(EditorWindow)

        GLFWwindow* GetNativeWindow() const;
        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        uint32_t GetFramebufferWidth() const;
        uint32_t GetFramebufferHeight() const;
        bool ShouldClose() const;
        void RequestClose() const;
        void PollEvents() const;
        void SetTitle(const std::string& inTitle) const;
        void SetDrawData(ImDrawData* inDrawData);
        void RecreateSwapChainIfNeeded();
        void WaitRenderingIdle() const;

        void RenderPendingUi();
        void RenderUiOnly(ImDrawData& inDrawData);

    private:
        struct ImGuiPassParams {
            Common::FVec2 displayPos;
            Common::FVec2 displaySize;
        };

        void* GetPlatformWindow() const;
        void RenderUiPass(ImDrawData* inDrawData, RHI::LoadOp inLoadOp, const Common::LinearColor& inClearValue);
        void RenderImGuiDrawData(ImDrawData& inDrawData, RHI::RasterPassCommandRecorder& inRecorder);
        void CreateImGuiDeviceObjects();
        void CreateImGuiFontTexture();
        void CreateImGuiDynamicBuffers(int inVertexCount, int inIndexCount);
        void UpdateImGuiBuffers(ImDrawData& inDrawData);
        void UpdateImGuiPassParams(const ImDrawData& inDrawData);
        RHI::BindGroup* GetOrCreateImGuiTextureBindGroup(RHI::TextureView& inTextureView);
        ImDrawData* TakeDrawData();
        static void DestroyDrawData(ImDrawData* inDrawData);

        GLFWwindow* window;
        Common::UniquePtr<RHI::Fence> uiFrameFence;
        Common::UniquePtr<RHI::CommandBuffer> uiCommandBuffer;
        Render::ShaderCompileOutput imguiVertexShaderCompileOutput;
        Render::ShaderCompileOutput imguiPixelShaderCompileOutput;
        Common::UniquePtr<RHI::ShaderModule> imguiVertexShader;
        Common::UniquePtr<RHI::ShaderModule> imguiPixelShader;
        Common::UniquePtr<RHI::BindGroupLayout> imguiBindGroupLayout;
        Common::UniquePtr<RHI::PipelineLayout> imguiPipelineLayout;
        Common::UniquePtr<RHI::RasterPipeline> imguiPipeline;
        Common::UniquePtr<RHI::Texture> imguiFontTexture;
        Common::UniquePtr<RHI::TextureView> imguiFontTextureView;
        Common::UniquePtr<RHI::Sampler> imguiImageSampler;
        Common::UniquePtr<RHI::Buffer> imguiPassParamsBuffer;
        Common::UniquePtr<RHI::BufferView> imguiPassParamsBufferView;
        std::unordered_map<RHI::TextureView*, Common::UniquePtr<RHI::BindGroup>> imguiFrameTextureBindGroups;
        Common::UniquePtr<RHI::Buffer> imguiVertexBuffer;
        Common::UniquePtr<RHI::BufferView> imguiVertexBufferView;
        Common::UniquePtr<RHI::Buffer> imguiIndexBuffer;
        Common::UniquePtr<RHI::BufferView> imguiIndexBufferView;
        int imguiVertexBufferCapacity;
        int imguiIndexBufferCapacity;
        uint32_t framebufferWidth;
        uint32_t framebufferHeight;
        mutable std::mutex drawDataMutex;
        ImDrawData* pendingDrawData;
    };
}

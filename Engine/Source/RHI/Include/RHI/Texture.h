//
// Created by johnk on 2022/1/23.
//

#pragma once

#include <Common/Utility.h>
#include <RHI/Common.h>
#include <string>

namespace RHI {
    struct TextureViewCreateInfo;
    class TextureView;
    class Device;

    struct TextureCreateInfo {
        TextureType type;
        uint32_t width;
        uint32_t height;
        uint32_t depthOrArraySize;
        PixelFormat format;
        TextureUsageFlags usages;
        uint8_t mipLevels;
        uint8_t samples;
        TextureState initialState;
        std::string debugName;

        TextureCreateInfo();
        TextureCreateInfo& SetType(TextureType inType);
        TextureCreateInfo& SetWidth(uint32_t inWidth);
        TextureCreateInfo& SetHeight(uint32_t inHeight);
        TextureCreateInfo& SetDepthOrArraySize(uint32_t inDepthOrArraySize);
        TextureCreateInfo& SetFormat(PixelFormat inFormat);
        TextureCreateInfo& SetUsages(TextureUsageFlags inUsages);
        TextureCreateInfo& SetMipLevels(uint8_t inMipLevels);
        TextureCreateInfo& SetSamples(uint8_t inSamples);
        TextureCreateInfo& SetInitialState(TextureState inState);
        TextureCreateInfo& SetDebugName(std::string inDebugName);

        bool operator==(const TextureCreateInfo& rhs) const;
    };

    class Texture {
    public:
        NonCopyable(Texture)
        virtual ~Texture();

        const TextureCreateInfo& GetCreateInfo() const;
        Common::UniquePtr<TextureView> CreateTextureView(const TextureViewCreateInfo& createInfo);

    protected:
        explicit Texture(const TextureCreateInfo& inCreateInfo);
        virtual Common::UniquePtr<TextureView> CreateTextureViewInternal(const TextureViewCreateInfo& createInfo) = 0;

        TextureCreateInfo createInfo;
    };
}

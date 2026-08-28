//
// Created by johnk on 2023/3/25.
//

#include <RHI/Common.h>

namespace RHI {
    size_t GetBytesPerPixel(PixelFormat format)
    {
        struct BytesPerPixelRange {
            PixelFormat begin;
            PixelFormat end;
            size_t bytesPerPixel;
        };
        static constexpr BytesPerPixelRange ranges[] = {
            { PixelFormat::begin8Bits,   PixelFormat::begin16Bits,  1 },
            { PixelFormat::begin16Bits,  PixelFormat::begin32Bits,  2 },
            { PixelFormat::begin32Bits,  PixelFormat::begin64Bits,  4 },
            { PixelFormat::begin64Bits,  PixelFormat::begin128Bits, 8 },
            { PixelFormat::begin128Bits, PixelFormat::max,          16 },
        };

        for (const auto& range : ranges) {
            if (format > range.begin && format < range.end) {
                return range.bytesPerPixel;
            }
        }
        return Assert(false), 1;
    }

    size_t GetTextureAspectBytesPerPixel(const PixelFormat format, const TextureAspect aspect)
    {
        const auto textureAspect = GetTextureAspect(format);
        if (textureAspect == TextureAspect::depthStencil) {
            Assert(aspect == TextureAspect::depth || aspect == TextureAspect::stencil);
            return aspect == TextureAspect::depth ? 4 : 1;
        }

        Assert(aspect == textureAspect);
        return GetBytesPerPixel(format);
    }

    TextureAspect GetTextureAspect(const PixelFormat format)
    {
        switch (format) {
            case PixelFormat::d16Unorm:
            case PixelFormat::d32Float:
                return TextureAspect::depth;
            case PixelFormat::d24UnormS8Uint:
            case PixelFormat::d32FloatS8Uint:
                return TextureAspect::depthStencil;
            default:
                return TextureAspect::color;
        }
    }

    std::span<const TextureAspect> GetTextureAspectComponents(const TextureAspect aspect)
    {
        static constexpr TextureAspect color[] = { TextureAspect::color };
        static constexpr TextureAspect depth[] = { TextureAspect::depth };
        static constexpr TextureAspect stencil[] = { TextureAspect::stencil };
        static constexpr TextureAspect depthStencil[] = { TextureAspect::depth, TextureAspect::stencil };

        switch (aspect) {
            case TextureAspect::color:
                return color;
            case TextureAspect::depth:
                return depth;
            case TextureAspect::stencil:
                return stencil;
            case TextureAspect::depthStencil:
                return depthStencil;
            default:
                return Assert(false), std::span<const TextureAspect>();
        }
    }

    TextureState GetDepthStencilTextureState(const TextureAspect aspect, const bool depthReadOnly, const bool stencilReadOnly)
    {
        if (aspect == TextureAspect::depth) {
            return depthReadOnly ? TextureState::depthStencilReadonly : TextureState::depthStencilWrite;
        }
        if (aspect == TextureAspect::stencil) {
            return stencilReadOnly ? TextureState::depthStencilReadonly : TextureState::depthStencilWrite;
        }

        Assert(aspect == TextureAspect::depthStencil);
        if (depthReadOnly && stencilReadOnly) {
            return TextureState::depthStencilReadonly;
        }
        if (depthReadOnly) {
            return TextureState::depthReadStencilWrite;
        }
        if (stencilReadOnly) {
            return TextureState::depthWriteStencilRead;
        }
        return TextureState::depthStencilWrite;
    }
}

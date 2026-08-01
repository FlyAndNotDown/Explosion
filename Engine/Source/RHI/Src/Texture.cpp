//
// Created by johnk on 2022/1/23.
//

#include <RHI/Texture.h>
#include <RHI/TextureView.h>

namespace RHI::Internal {
    static void ValidateTextureCreateInfo(const TextureCreateInfo& createInfo)
    {
        switch (createInfo.type) {
            case TextureType::t1D:
            case TextureType::t2D:
                AssertWithReason(createInfo.depthOrArraySize == 1, "non-array textures must have exactly one array layer");
                break;
            case TextureType::t2DArray:
                AssertWithReason(createInfo.depthOrArraySize >= 1, "texture arrays must have at least one array layer");
                break;
            case TextureType::tCube:
                AssertWithReason(createInfo.width == createInfo.height, "cube textures must be square");
                AssertWithReason(createInfo.depthOrArraySize == textureCubeFaceNum, "cube textures must have exactly six array layers");
                AssertWithReason(createInfo.samples == 1, "cube textures must be single-sampled");
                break;
            case TextureType::tCubeArray:
                AssertWithReason(createInfo.width == createInfo.height, "cube texture arrays must be square");
                AssertWithReason(createInfo.depthOrArraySize >= textureCubeFaceNum && createInfo.depthOrArraySize % textureCubeFaceNum == 0, "cube texture arrays must have a positive multiple of six array layers");
                AssertWithReason(createInfo.samples == 1, "cube texture arrays must be single-sampled");
                break;
            case TextureType::t3D:
                AssertWithReason(createInfo.depthOrArraySize >= 1, "3D textures must have positive depth");
                break;
            default:
                break;
        }
    }

    static bool IsTextureViewDimensionCompatible(TextureType textureType, TextureViewDimension viewDimension)
    {
        switch (textureType) {
            case TextureType::t1D:
                return viewDimension == TextureViewDimension::tv1D;
            case TextureType::t2D:
                return viewDimension == TextureViewDimension::tv2D;
            case TextureType::t2DArray:
                return viewDimension == TextureViewDimension::tv2D || viewDimension == TextureViewDimension::tv2DArray;
            case TextureType::tCube:
                return viewDimension == TextureViewDimension::tv2D || viewDimension == TextureViewDimension::tv2DArray || viewDimension == TextureViewDimension::tvCube;
            case TextureType::tCubeArray:
                return viewDimension == TextureViewDimension::tv2D || viewDimension == TextureViewDimension::tv2DArray || viewDimension == TextureViewDimension::tvCube || viewDimension == TextureViewDimension::tvCubeArray;
            case TextureType::t3D:
                return viewDimension == TextureViewDimension::tv3D;
            default:
                return false;
        }
    }

    static void ValidateTextureViewCreateInfo(const TextureCreateInfo& textureCreateInfo, const TextureViewCreateInfo& viewCreateInfo)
    {
        AssertWithReason(IsTextureViewDimensionCompatible(textureCreateInfo.type, viewCreateInfo.dimension), "texture view dimension is incompatible with the texture type");
        AssertWithReason(viewCreateInfo.mipLevelNum > 0 && viewCreateInfo.baseMipLevel + viewCreateInfo.mipLevelNum <= textureCreateInfo.mipLevels, "texture view mip range is out of bounds");

        const auto arrayLayerNum = textureCreateInfo.type == TextureType::t3D ? 1 : textureCreateInfo.depthOrArraySize;
        AssertWithReason(viewCreateInfo.arrayLayerNum > 0 && viewCreateInfo.baseArrayLayer + viewCreateInfo.arrayLayerNum <= arrayLayerNum, "texture view array layer range is out of bounds");

        if (viewCreateInfo.dimension == TextureViewDimension::tv1D || viewCreateInfo.dimension == TextureViewDimension::tv2D || viewCreateInfo.dimension == TextureViewDimension::tv3D) {
            AssertWithReason(viewCreateInfo.arrayLayerNum == 1, "non-array texture views must have exactly one array layer");
        } else if (viewCreateInfo.dimension == TextureViewDimension::tvCube) {
            AssertWithReason(viewCreateInfo.baseArrayLayer % textureCubeFaceNum == 0 && viewCreateInfo.arrayLayerNum == textureCubeFaceNum, "cube texture views must select one aligned group of six array layers");
        } else if (viewCreateInfo.dimension == TextureViewDimension::tvCubeArray) {
            AssertWithReason(viewCreateInfo.baseArrayLayer % textureCubeFaceNum == 0 && viewCreateInfo.arrayLayerNum % textureCubeFaceNum == 0, "cube texture array views must select aligned groups of six array layers");
        }
    }
}

namespace RHI {
    TextureCreateInfo::TextureCreateInfo()
        : type(TextureType::max)
        , width(0)
        , height(0)
        , depthOrArraySize(0)
        , format(PixelFormat::max)
        , usages(TextureUsageFlags::null)
        , mipLevels(0)
        , samples(1)
        , initialState(TextureState::max)
    {
    }

    TextureCreateInfo& TextureCreateInfo::SetType(const TextureType inType)
    {
        type = inType;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetWidth(const uint32_t inWidth)
    {
        width = inWidth;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetHeight(const uint32_t inHeight)
    {
        height = inHeight;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetDepthOrArraySize(const uint32_t inDepthOrArraySize)
    {
        depthOrArraySize = inDepthOrArraySize;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetFormat(const PixelFormat inFormat)
    {
        format = inFormat;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetUsages(const TextureUsageFlags inUsages)
    {
        usages = inUsages;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetMipLevels(const uint8_t inMipLevels)
    {
        mipLevels = inMipLevels;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetSamples(const uint8_t inSamples)
    {
        samples = inSamples;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetInitialState(const TextureState inState)
    {
        initialState = inState;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::SetDebugName(std::string inDebugName)
    {
        debugName = std::move(inDebugName);
        return *this;
    }

    bool TextureCreateInfo::operator==(const TextureCreateInfo& rhs) const
    {
        return type == rhs.type
            && width == rhs.width
            && height == rhs.height
            && depthOrArraySize == rhs.depthOrArraySize
            && format == rhs.format
            && usages == rhs.usages
            && mipLevels == rhs.mipLevels
            && samples == rhs.samples
            && initialState == rhs.initialState;
    }

    Texture::Texture(const TextureCreateInfo& inCreateInfo)
        : createInfo(inCreateInfo)
    {
        Internal::ValidateTextureCreateInfo(inCreateInfo);
    }

    Texture::~Texture() = default;

    const TextureCreateInfo& Texture::GetCreateInfo() const
    {
        return createInfo;
    }

    Common::UniquePtr<TextureView> Texture::CreateTextureView(const TextureViewCreateInfo& inCreateInfo)
    {
        Internal::ValidateTextureViewCreateInfo(createInfo, inCreateInfo);
        return CreateTextureViewInternal(inCreateInfo);
    }
}

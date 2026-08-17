//
// Created by johnk on 13/1/2022.
//

#include <RHI/DirectX12/Common.h>

namespace RHI::DirectX12 {
    uint32_t GetDX12TexturePlaneSlice(const TextureAspect aspect)
    {
        switch (aspect) {
            case TextureAspect::color:
            case TextureAspect::depth:
                return 0;
            case TextureAspect::stencil:
                return 1;
            default:
                return Assert(false), 0;
        }
    }
}

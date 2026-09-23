#include "FTexture.h"

size_t FTexture::GetMemorySize() const
{
    size_t TotalSize = 0;

    uint32 MipWidth = Width;
    uint32 MipHeight = Height;

    for (uint32 Mip = 0; Mip < MipLevels; ++Mip)
    {
        switch (Format)
        {
            // ------------------------------------------------------------
            // Uncompressed
            // ------------------------------------------------------------

        case DXGI_FORMAT_R8_UNORM:
            TotalSize += static_cast<size_t>(MipWidth) *
                MipHeight *
                1;
            break;

        case DXGI_FORMAT_R8G8_UNORM:
            TotalSize += static_cast<size_t>(MipWidth) *
                MipHeight *
                2;
            break;

        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            TotalSize += static_cast<size_t>(MipWidth) *
                MipHeight *
                4;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            TotalSize += static_cast<size_t>(MipWidth) *
                MipHeight *
                8;
            break;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            TotalSize += static_cast<size_t>(MipWidth) *
                MipHeight *
                16;
            break;


            // ------------------------------------------------------------
            // BC1 / BC4
            // 4x4 pixels = 8 bytes
            // ------------------------------------------------------------

        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        {
            const uint32 BlockWidth =
                std::max(1u, (MipWidth + 3) / 4);

            const uint32 BlockHeight =
                std::max(1u, (MipHeight + 3) / 4);

            TotalSize += static_cast<size_t>(BlockWidth) *
                BlockHeight *
                8;
            break;
        }


        // ------------------------------------------------------------
        // BC2 / BC3 / BC5 / BC6H / BC7
        // 4x4 pixels = 16 bytes
        // ------------------------------------------------------------

        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:

        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:

        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:

        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:

        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        {
            const uint32 BlockWidth =
                std::max(1u, (MipWidth + 3) / 4);

            const uint32 BlockHeight =
                std::max(1u, (MipHeight + 3) / 4);

            TotalSize += static_cast<size_t>(BlockWidth) *
                BlockHeight *
                16;
            break;
        }

        default:
            return 0;
        }

        MipWidth = std::max(1u, MipWidth / 2);
        MipHeight = std::max(1u, MipHeight / 2);
    }

    return TotalSize;
}
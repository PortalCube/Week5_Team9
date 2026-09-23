#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "Runtime/Core/IntTypes.h"

class FTexture final
{
	friend class FRenderer;
	friend class FMaterial;

public:
	[[nodiscard]] ID3D11ShaderResourceView* GetSRV() const { return TextureSRV.Get(); }
	[[nodiscard]] uint32 GetWidth() const { return Width; }
	[[nodiscard]] uint32 GetHeight() const { return Height; }
	[[nodiscard]] size_t GetMemorySize() const;
private:
	FTexture() = default;
	
	uint32 Width = 0u;
	uint32 Height = 0u;
	uint32 MipLevels = 0u;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
	size_t MemorySize = 0;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture2D;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureSRV;
};

struct FTextureDesc
{
	const void* PixelData = nullptr;
	uint32 Width = 0u;
	uint32 Height = 0u;
	uint32 RowPitch = 0u;
};

#pragma once

#include "Runtime/Core/IntTypes.h"

// 사실 더 쉽게 가고 싶으면 D3D11_FILTER의 값과 매칭 시켜도 괜찮음

// 텍스쳐의 Filter 모드
enum class ETextureSamplerFilterMode : uint8
{
	Point		= 0, // D3D11_FILTER_MIN_MAG_MIP_POINT
	Bilinear	= 1, // D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT
	Trilinear	= 2, // D3D11_FILTER_MIN_MAG_MIP_LINEAR
	Anisotropic	= 3, // D3D11_FILTER_ANISOTROPIC
};

// 텍스쳐의 Wrap 모드 (Address)
enum class ETextureSamplerWrapMode : uint8
{
	Wrap	= 0, // D3D11_TEXTURE_ADDRESS_WRAP
	Mirror	= 1, // D3D11_TEXTURE_ADDRESS_MIRROR 
	Clamp	= 2, // D3D11_TEXTURE_ADDRESS_CLAMP 
};

struct FTextureSamplerDesc
{
	ETextureSamplerFilterMode FilterMode	= ETextureSamplerFilterMode::Bilinear;
	ETextureSamplerWrapMode WrapMode		= ETextureSamplerWrapMode::Wrap;

	bool operator==(const FTextureSamplerDesc&) const = default;
};

namespace std
{
	template <>
	struct hash<FTextureSamplerDesc>
	{
		size_t operator()(const FTextureSamplerDesc& Desc) const noexcept
		{
			return static_cast<size_t>(Desc.FilterMode)
				| (static_cast<size_t>(Desc.WrapMode) << 8);
		}
	};
}

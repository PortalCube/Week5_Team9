#pragma once

#include "Runtime/Core/IntTypes.h"

enum class ERasterizerFillMode : uint8
{
	Solid		= 0,
	Wireframe	= 1,
};

enum class ERasterizerCullMode : uint8
{
	None	= 0,
	Front	= 1,
	Back	= 2,
};

enum class ERasterizerFrontFaceMode : uint8
{
	CounterClockwise	= 0,
	Clockwise			= 1,
};

struct FRasterizerDesc
{
	ERasterizerFillMode FillMode		= ERasterizerFillMode::Solid;
	ERasterizerCullMode CullMode		= ERasterizerCullMode::Back;
	ERasterizerFrontFaceMode FrontFace	= ERasterizerFrontFaceMode::CounterClockwise;
	bool bUseMultisample				= false;
	bool bUseAntialiasedLine			= false;

	bool operator==(const FRasterizerDesc&) const = default;
};

namespace std
{
	template <>
	struct hash<FRasterizerDesc>
	{
		size_t operator()(const FRasterizerDesc& Desc) const noexcept
		{
			return static_cast<size_t>(Desc.FillMode)
				| (static_cast<size_t>(Desc.CullMode) << 8)
				| (static_cast<size_t>(Desc.FrontFace) << 16)
				| (static_cast<size_t>(Desc.bUseMultisample) << 24)
				| (static_cast<size_t>(Desc.bUseAntialiasedLine) << 25);
		}
	};
}

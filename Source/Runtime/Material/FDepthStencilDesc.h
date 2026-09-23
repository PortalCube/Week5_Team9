#pragma once

#include "Runtime/Core/IntTypes.h"

enum class EDepthWriteMode : uint8
{
	Disable	= 0,
	Enable	= 1,
};

struct FDepthStencilDesc
{
	bool bDepthEnable			= true;
	bool bStencilEnable			= true;
	EDepthWriteMode DepthWrite	= EDepthWriteMode::Enable;

	bool operator==(const FDepthStencilDesc&) const = default;
};

namespace std
{
	template <>
	struct hash<FDepthStencilDesc>
	{
		size_t operator()(const FDepthStencilDesc& Desc) const noexcept
		{
			return static_cast<size_t>(Desc.bDepthEnable)
				| (static_cast<size_t>(Desc.bStencilEnable) << 1)
				| (static_cast<size_t>(Desc.DepthWrite) << 8);
		}
	};
}

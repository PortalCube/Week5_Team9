#pragma once

#include "Runtime/Core/IntTypes.h"

enum class EBlendMode : uint8
{
	Opaque,
	Masked,
	Translucent,
	Additive,
	PremultipliedAlpha,
};

struct FBlendDesc
{
	EBlendMode BlendMode = EBlendMode::Opaque;

	bool operator==(const FBlendDesc&) const = default;
};

namespace std
{
	template <>
	struct hash<FBlendDesc>
	{
		size_t operator()(const FBlendDesc& Desc) const noexcept
		{
			return static_cast<size_t>(Desc.BlendMode);
		}
	};
}

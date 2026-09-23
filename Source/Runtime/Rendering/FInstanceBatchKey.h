#pragma once

#include "Runtime/Core/FName.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"

#include <functional>

struct FInstanceBatchKey
{
	const FMesh* Mesh = nullptr;
	const FMaterial* Material = nullptr;

	bool operator==(const FInstanceBatchKey& Other) const = default;
};

template <>
struct std::hash<FInstanceBatchKey>
{
	size_t operator()(const FInstanceBatchKey& Key) const noexcept
	{
		const size_t MaterialHash = std::hash<const FMesh*>{}(Key.Mesh);
		const size_t MeshHash = std::hash<const FMaterial*>{}(Key.Material);
		return EngineUtil::HashCombine(MaterialHash, MeshHash);
	}
};

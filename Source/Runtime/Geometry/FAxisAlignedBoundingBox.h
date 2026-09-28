#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Core/TArray.h"

#include <limits>

class FMesh;
struct FMatrix;

struct FAxisAlignedBoundingBox
{
	FVector Center{};
	FVector Extent{};

	FVector Min
	{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
	};

	FVector Max
	{
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
	};

	FAxisAlignedBoundingBox() = default;
	FAxisAlignedBoundingBox(const FAxisAlignedBoundingBox& InBounds, const FMatrix& TransformationMatrix);
	FAxisAlignedBoundingBox(const FMesh& Mesh);
	FAxisAlignedBoundingBox(const FMesh& Mesh, const FMatrix& ModelMatrix);
};
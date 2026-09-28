#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Core/TArray.h"

#include <limits>

class FMesh;
struct FMatrix;

struct FAxisAlignedBoundingBox
{
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
	FAxisAlignedBoundingBox(const FAxisAlignedBoundingBox& InBounds, const FMatrix& ModelMatrix);
	FAxisAlignedBoundingBox(const FMesh& Mesh);
	FAxisAlignedBoundingBox(const FMesh& Mesh, const FMatrix& ModelMatrix);

	void GetCorner(FVector OutCorner[8]) const;

	[[nodiscard]] bool IsValid() const
	{
		return Min.X <= Max.X && Min.Y <= Max.Y && Min.Z <= Max.Z;
	}

	[[nodiscard]] static FAxisAlignedBoundingBox Union(const FAxisAlignedBoundingBox& A, const FAxisAlignedBoundingBox& B);
	bool operator==(const FAxisAlignedBoundingBox& Other) const
	{
		return Min.X == Other.Min.X && Min.Y == Other.Min.Y && Min.Z == Other.Min.Z
			&& Max.X == Other.Max.X && Max.Y == Other.Max.Y && Max.Z == Other.Max.Z;
	}
};
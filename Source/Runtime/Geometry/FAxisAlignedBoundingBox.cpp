#include "FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FMatrix.h"

#include <algorithm>

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FAxisAlignedBoundingBox& InBounds, const FMatrix& ModelMatrix)
{
	FVector Corner[8];
	InBounds.GetCorner(Corner);

	for (int i = 0; i < 8; i++)
	{
		FVector WorldVector = ModelMatrix.TransformPointRow(Corner[i]);

		for (int j = 0; j < 3;j++)
		{
			Min[j] = std::min(WorldVector[j], Min[j]);
			Max[j] = std::max(WorldVector[j], Max[j]);
		}
	}
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FMesh& Mesh)
{	
	for (auto& Item : Mesh.GetPositions())
	{
		for (int i = 0; i < 3; ++i)
		{
			Min[i] = std::min(Item[i], Min[i]);
			Max[i] = std::max(Item[i], Max[i]);
		}
	}
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FMesh& Mesh, const FMatrix& ModelMatrix)
	: FAxisAlignedBoundingBox(Mesh.GetLocalBounds(), ModelMatrix)
{
}

void FAxisAlignedBoundingBox::GetCorner(FVector OutCorner[8]) const
{
	// Bottom
	OutCorner[0] = FVector{ Min.X, Min.Y, Min.Z };
	OutCorner[1] = FVector{ Min.X, Max.Y, Min.Z };
	OutCorner[2] = FVector{ Max.X, Min.Y, Min.Z };
	OutCorner[3] = FVector{ Max.X, Max.Y, Min.Z };
	// Top
	OutCorner[4] = FVector{ Min.X, Min.Y, Max.Z };
	OutCorner[5] = FVector{ Min.X, Max.Y, Max.Z };
	OutCorner[6] = FVector{ Max.X, Min.Y, Max.Z };
	OutCorner[7] = FVector{ Max.X, Max.Y, Max.Z };
}

//FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FMesh& Mesh, const FMatrix& ModelMatrix)
//{
//	for (auto& Item : Mesh.GetPositions())
//	{
//		FVector WorldVector = ModelMatrix.TransformPointRow(Item);
//
//		for (int i = 0; i < 3; ++i)
//		{
//			Min[i] = std::min(WorldVector[i], Min[i]);
//			Max[i] = std::max(WorldVector[i], Max[i]);
//		}	
//	}
//}
#include "FAxisAlignedBoundingBox.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FMatrix.h"

#include <algorithm>

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FAxisAlignedBoundingBox& InBounds, const FMatrix& TransformationMatrix)
{
	Center = TransformationMatrix.TransformPointRow(InBounds.Center);
	Extent = TransformationMatrix.Abs().TransformPointRow(InBounds.Extent, 0.0f);

	Min = Center - Extent;
	Max = Center + Extent;
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FMesh& Mesh)
{	
	if (Mesh.GetPositions().empty())
	{
		return;
	}

	for (auto& Item : Mesh.GetPositions())
	{
		for (int i = 0; i < 3; ++i)
		{
			Min[i] = std::min(Item[i], Min[i]);
			Max[i] = std::max(Item[i], Max[i]);
		}
	}

	Center = (Max + Min) / 2;
	Extent = Center - Min;
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FMesh& Mesh, const FMatrix& ModelMatrix)
	: FAxisAlignedBoundingBox(Mesh.GetLocalBounds(), ModelMatrix)
{
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FVector& pCenter, const FVector& pExtent)
{
	Center = pCenter;
	Extent = pExtent;

	Min = Center - Extent;
	Max = Center + Extent;
}

FAxisAlignedBoundingBox::FAxisAlignedBoundingBox(const FVector& _Min, const FVector& _Max)
{
	Min = _Min;
	Max = _Max;
	Center = (Min + Max) / 2;
	Extent = Center - Min;
}

FAxisAlignedBoundingBox FAxisAlignedBoundingBox::Union(const FAxisAlignedBoundingBox& A, const FAxisAlignedBoundingBox& B)
{
	FAxisAlignedBoundingBox R;

	//두 AABB를 품을 수 있는 크기로 Min, Max를 재조정한다.
	for (int i = 0; i < 3; ++i)
	{
		R.Min[i] = std::min(A.Min[i], B.Min[i]);
		R.Max[i] = std::max(A.Max[i], B.Max[i]);
	}

	R.Center = (R.Max + R.Min) / 2;
	R.Extent = R.Center - R.Min;

	return R;
}
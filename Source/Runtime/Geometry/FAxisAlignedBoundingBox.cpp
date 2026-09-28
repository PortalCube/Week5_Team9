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
#include "FBillboardVisualizer.h"

#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Core/TArray.h"

void FBillboardVisualizer::Draw(
	const UPrimitiveComponent& Component,
	FRenderView& RenderView,
	const FCamera& Camera,
	const FVector4& Color
) const
{
	if (Component.IsA<UBillBoardComp>() == false) { return; }

	const UBillBoardComp& BillBoardComponent = *Component.Cast<UBillBoardComp>();

	UStaticMesh* MeshPtr = BillBoardComponent.GetRenderData(Camera).Mesh;
	if (!MeshPtr) return;
	const FMesh& Mesh = *MeshPtr->Get();
	const FMatrix ModelMatrix = BillBoardComponent.GetRenderMatrix(Camera);

	if (Mesh.GetPositions().size() != 4) { return; }

	TArray<FVector> Array;
	for (int i = 0; i < 4; ++i)
	{
		FVector WorldVector = ModelMatrix.TransformPointRow(Mesh.GetPositions()[i]);
		Array.push_back(WorldVector);
	}

	RenderView.RenderQuad(
		Array[0],
		Array[1],
		Array[2],
		Array[3],
		Color
	);
}

#include "FPrimitiveVisualizer.h"

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/FMatrix.h"

void FPrimitiveVisualizer::Draw(
	const UPrimitiveComponent& Component,
	FRenderView& RenderView,
	const FCamera& Camera,
	const FVector4& Color
) const
{
    if (Component.IsA<UPrimitiveComponent>() == false) { return; }

    UStaticMesh* Mesh = Component.GetRenderData(Camera).Mesh;
    if (!Mesh) return;
    const FMatrix ModelMatrix = Component.GetRenderMatrix(Camera);
    FAxisAlignedBoundingBox AABB{ *Mesh->Get(), ModelMatrix};
    RenderView.RenderBoxMinMax(AABB.Min, AABB.Max, Color);
}

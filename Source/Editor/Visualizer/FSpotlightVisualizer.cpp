#include "FSpotlightVisualizer.h"

#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/FMatrix.h"

void FSpotlightVisualizer::Draw(
	const UPrimitiveComponent& Component,
	FRenderView& RenderView,
	const FCamera& Camera,
	const FVector4& Color
) const
{
    if (Component.IsA<USpotLightComponent>() == false) { return; }

    auto MeshPtr = Component.GetRenderData(Camera).Mesh;
    if (!MeshPtr) return;
    const FMesh& Mesh = *MeshPtr->Get();
    const FMatrix ModelMatrix = Component.GetRenderMatrix(Camera);

    const auto& Positions = Mesh.GetPositions();
    const auto& Indices = Mesh.GetIndices();

    const FVector4 LightColor{ 1.0f, 1.0f, 0.0f, 1.0f }; // 조명 전용 노란색

    // 메쉬의 삼각형 인덱스를 순회하며 모서리 선 그리기
    for (size_t i = 0; i + 2 < Indices.size(); i += 3)
    {
        FVector A = ModelMatrix.TransformPointRow(Positions[Indices[i]]);
        FVector B = ModelMatrix.TransformPointRow(Positions[Indices[i + 1]]);
        FVector C = ModelMatrix.TransformPointRow(Positions[Indices[i + 2]]);

        RenderView.RenderLine(A, B, LightColor);
        RenderView.RenderLine(B, C, LightColor);
        RenderView.RenderLine(C, A, LightColor);
    }

    FAxisAlignedBoundingBox AABB{ Mesh, ModelMatrix };
    RenderView.RenderBoxMinMax(AABB.Min, AABB.Max, Color);
}

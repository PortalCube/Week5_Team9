#include "FTextVisualizer.h"

#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"

void FTextVisualizer::Draw(
	const UPrimitiveComponent& Component,
	FRenderView& RenderView,
	const FCamera& Camera,
	const FVector4& Color
) const
{
	if (Component.IsA<UTextInstanceComponent>() == false) { return; }



  const UTextInstanceComponent &TextComponent =
      *Component.Cast<UTextInstanceComponent>();

  float Width = TextComponent.GetWidth();
  float Height = TextComponent.GetHeight();

  UStaticMesh* MeshPtr = TextComponent.GetRenderData(Camera).Mesh;
  if (!MeshPtr)
    return;
  const FMesh &Mesh = *MeshPtr->Get();
  const FMatrix ModelMatrix = TextComponent.GetRenderMatrix(Camera);

  if (Mesh.GetPositions().size() != 4) {
    return;
  }

  TArray<FVector> Array;
  for (int i = 0; i < 4; ++i) {
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

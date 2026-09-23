#include "FGrid.h"

#include "Editor/Core/FEditor.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector4.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include <cmath>
#include <numbers>


#include "Runtime/Engine/FRayCastingManager.h"


void FGrid::DrawLine(FRenderer &Renderer, const FCamera &Camera) {
  auto &LineBatcher = Renderer.GetLineBatcher();

  const int32 HalfLineCount = static_cast<int32>(500.0f / CellSize);
  const float Extent = HalfLineCount * CellSize;

  const float SnapX = std::floor(Camera.Position.X / CellSize) * CellSize;
  const float SnapY = std::floor(Camera.Position.Y / CellSize) * CellSize;

  const FVector4 MinorGridColor{0.1f, 0.1f, 0.1f, 1.0f};
  const FVector4 MajorGridColor{0.3f, 0.3f, 0.3f, 1.0f};
  const FVector4 AxisColorX{1.0f, 0.0f, 0.0f, 1.0f};
  const FVector4 AxisColorY{0.0f, 1.0f, 0.0f, 1.0f};
  const FVector4 BackgroundColor{0.05f, 0.05f, 0.08f, 1.0f};

  //const bool bEnableMajorGrid = (CellSize <= 0.2f);
  const float LineOffset = CellSize * 0.02f;

  // 가로선 렌더링
  for (int32 i = -HalfLineCount; i <= HalfLineCount; ++i) {
    float Y = SnapY + i * CellSize;

    if (std::abs(Y) < 0.001f) {
      LineBatcher.DrawLine(FVector{SnapX - Extent, Y, 0.0f},
                           FVector{SnapX + Extent, Y, 0.0f}, AxisColorX);
    } else {
      const int64 GridIndex = static_cast<int64>(std::round(Y / CellSize));
      const bool bIsMajor = (std::abs(GridIndex) % 10 == 0);
      const FVector4 Color = bIsMajor ? MajorGridColor : MinorGridColor;

      LineBatcher.DrawLine(FVector{SnapX - Extent, Y, 0.0f},
                           FVector{SnapX + Extent, Y, 0.0f}, Color);
    }
  }

  // 세로선 렌더링
  for (int32 i = -HalfLineCount; i <= HalfLineCount; ++i) {
    float X = SnapX + i * CellSize;
    if (std::abs(X) < 0.001f) {
      LineBatcher.DrawLine(FVector{X, SnapY - Extent, 0.0f},
                           FVector{X, SnapY + Extent, 0.0f}, AxisColorY);
    } else {
      const int64 GridIndex = static_cast<int64>(std::round(X / CellSize));
      const bool bIsMajor = (std::abs(GridIndex) % 10 == 0);
      const FVector4 Color = bIsMajor ? MajorGridColor : MinorGridColor;

      LineBatcher.DrawLine(FVector{X, SnapY - Extent, 0.0f},
                           FVector{X, SnapY + Extent, 0.0f}, Color);
    }
  }

  // 수직 축선 렌더링
  LineBatcher.DrawLine(FVector{0.0f, 0.0f, -Extent},
                       FVector{0.0f, 0.0f, Extent},
                       FVector4{0.0f, 0.0f, 1.0f, 1.0f});
}

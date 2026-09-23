#include "MeshUtil.h"

#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Core/TArray.h"
#include <cmath>
#include <numbers>

namespace
{
TSharedPtr<FMesh> CreateInternalMesh(FRenderer& Renderer, FMeshDesc Desc)
{
  if (Desc.Sections.empty())
  {
    Desc.Sections.push_back(FMeshSection{
        .SectionName = "",
        .StartIndex = 0,
        .IndexCount = Desc.IndexCount,
    });
  }

  return Renderer.CreateMesh(Desc);
}
}

bool MeshUtil::CreateSpotlightConeMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr float BottomRadius = 0.5f;
  constexpr float Height = 1.0f;
  constexpr uint32 SliceCount = 48;
  constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
  const float DTheta = TAU / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  const float HalfH = Height * 0.5f;
  const float SlantLen =
      std::sqrt(Height * Height + BottomRadius * BottomRadius);
  const float NormalFactor = Height / SlantLen;
  const float NormalZ = BottomRadius / SlantLen;

  // 옆면 정점만 생성하고 밑면 뚜껑은 생성하지 않음
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    const float NextTheta = static_cast<float>(i + 1) * DTheta;
    const float MidTheta = (Theta + NextTheta) * 0.5f;

    const float ApexNx = NormalFactor * std::cos(MidTheta);
    const float ApexNy = NormalFactor * std::sin(MidTheta);

    const uint32 ApexIdx = static_cast<uint32>(Vertices.size());
    Vertices.push_back({0.0f, 0.0f, HalfH, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.0f,
                        ApexNx, ApexNy, NormalZ});

    const float BaseNx1 = NormalFactor * std::cos(Theta);
    const float BaseNy1 = NormalFactor * std::sin(Theta);
    const uint32 BaseIdx1 = static_cast<uint32>(Vertices.size());
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 1.0f, 1.0f,
                        1.0f, 1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        1.0f, BaseNx1, BaseNy1, NormalZ});

    const float BaseNx2 = NormalFactor * std::cos(NextTheta);
    const float BaseNy2 = NormalFactor * std::sin(NextTheta);
    const uint32 BaseIdx2 = static_cast<uint32>(Vertices.size());
    Vertices.push_back(
        {BottomRadius * std::cos(NextTheta), BottomRadius * std::sin(NextTheta),
         -HalfH, 1.0f, 1.0f, 1.0f, 1.0f,
         static_cast<float>(i + 1) / static_cast<float>(SliceCount), 1.0f,
         BaseNx2, BaseNy2, NormalZ});

    Indices.push_back(ApexIdx);
    Indices.push_back(BaseIdx2);
    Indices.push_back(BaseIdx1);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#SpotlightCone"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#SpotlightCone")] != nullptr;
}

bool MeshUtil::CreateArrowMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr uint32 SliceCount = 16u;
  constexpr float ShaftLength = 0.75f;
  constexpr float ShaftRadius = 0.025f;
  constexpr float HeadRadius = 0.075f;
  constexpr float HeadLength = 0.25f;
  constexpr float DTheta =
      2.0f * std::numbers::pi_v<float> / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 5 + 3);
  Indices.reserve(SliceCount * 18);

  const uint32 ShaftBottomCenter = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f,
                      -1.0f, 0.0f, 0.0f});

  const uint32 ShaftBottomRing = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({0.0f, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, -1.0f, 0.0f, 0.0f});
  }

  const uint32 ShaftSideBottom = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({0.0f, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 ShaftSideTop = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, ShaftRadius * std::cos(Theta),
                        ShaftRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 HeadBaseCenter = static_cast<uint32>(Vertices.size());
  Vertices.push_back({ShaftLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f,
                      0.5f, -1.0f, 0.0f, 0.0f});

  const uint32 HeadBaseRing = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, HeadRadius * std::cos(Theta),
                        HeadRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, -1.0f, 0.0f, 0.0f});
  }

  const uint32 HeadSideBase = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({ShaftLength, HeadRadius * std::cos(Theta),
                        HeadRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 0.0f, std::cos(Theta), std::sin(Theta)});
  }

  const uint32 HeadTip = static_cast<uint32>(Vertices.size());
  Vertices.push_back({ShaftLength + HeadLength, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                      1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f});

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(ShaftBottomCenter);
    Indices.push_back(ShaftBottomRing + Next);
    Indices.push_back(ShaftBottomRing + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    const uint32 BL = ShaftSideBottom + i;
    const uint32 BR = ShaftSideBottom + Next;
    const uint32 TL = ShaftSideTop + i;
    const uint32 TR = ShaftSideTop + Next;

    Indices.push_back(BL);
    Indices.push_back(TL);
    Indices.push_back(BR);

    Indices.push_back(BR);
    Indices.push_back(TL);
    Indices.push_back(TR);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(HeadBaseCenter);
    Indices.push_back(HeadBaseRing + Next);
    Indices.push_back(HeadBaseRing + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(HeadSideBase + i);
    Indices.push_back(HeadTip);
    Indices.push_back(HeadSideBase + Next);
  }

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#Arrow"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Arrow")] != nullptr;
}

bool MeshUtil::CreateCircleMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr uint32 SliceCount = 32u;
  constexpr float Radius = 1.0f;
  constexpr float Width = 0.07f;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 2);
  Indices.reserve(SliceCount * 6);

  for (uint32 i = 0; i < SliceCount; ++i) {
    float Theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                  static_cast<float>(SliceCount);
    float InnerRadius = Radius - Width * 0.5f;
    float OuterRadius = Radius + Width * 0.5f;

    Vertices.push_back({0.0f, InnerRadius * std::cos(Theta),
                        InnerRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f, 0.0f, 0.0f});
    Vertices.push_back({0.0f, OuterRadius * std::cos(Theta),
                        OuterRadius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f,
                        1.0f, 1.0f, 1.0f, 0.0f, 0.0f});

    uint32 InnerCurrent = 2 * i;
    uint32 OuterCurrent = 2 * i + 1;
    uint32 InnerNext = (2 * (i + 1)) % (SliceCount * 2);
    uint32 OuterNext = (2 * (i + 1) + 1) % (SliceCount * 2);

    Indices.push_back(InnerCurrent);
    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerNext);

    Indices.push_back(InnerNext);
    Indices.push_back(OuterCurrent);
    Indices.push_back(OuterNext);

    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerCurrent);
    Indices.push_back(InnerNext);

    Indices.push_back(OuterCurrent);
    Indices.push_back(InnerNext);
    Indices.push_back(OuterNext);
  }

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#Circle"), CreateInternalMesh(Renderer, Desc));
  return Library.AllMeshMap[FName("#Circle")] != nullptr;
}

bool MeshUtil::CreateRotationGizmoMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr uint32 SliceCount = 32u;
  constexpr float Radius = 1.0f;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 2);
  Indices.reserve(SliceCount * 6);

  for (uint32 i = 0; i < SliceCount; ++i) {
    float Theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                  static_cast<float>(SliceCount);

    Vertices.push_back({0.0f, Radius * std::cos(Theta),
                        Radius * std::sin(Theta), -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f, 0.0f});
    Vertices.push_back({0.0f, Radius * std::cos(Theta),
                        Radius * std::sin(Theta), 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 0.0f, 0.0f});

    if (i == 0)
      continue;

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i + 1u);
    Indices.push_back(2u * i - 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i + 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i - 1u);
    Indices.push_back(2u * i + 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i - 1u);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i + 1u);
    Indices.push_back(2u * i);

    Indices.push_back(2u * i - 2u);
    Indices.push_back(2u * i);
    Indices.push_back(2u * i + 1u);
  }
  Indices[0] = 2u * SliceCount - 2u;
  Indices[1] = 2u * SliceCount - 1u;
  Indices[3] = 2u * SliceCount - 2u;
  Indices[5] = 2u * SliceCount - 1u;
  Indices[6] = 2u * SliceCount - 2u;
  Indices[9] = 2u * SliceCount - 2u;

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#RotGizmo"), CreateInternalMesh(Renderer, Desc));
  return Library.AllMeshMap[FName("#RotGizmo")] != nullptr;
}

bool MeshUtil::CreateSquareArrowMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr float ShaftLength = 0.85f;
  constexpr float ShaftRadius = 0.025f;
  constexpr float ArrowLength = 1.0f;
  constexpr float TipSize = ArrowLength - ShaftLength;

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(16u);
  Indices.reserve(72u);

  for (const auto &v : ColoredCubeVertices) {
    float ScaledX = v.x * ShaftLength;
    float ScaledY = v.y * ShaftRadius;
    float ScaledZ = v.z * ShaftRadius;
    Vertices.push_back({ScaledX + ShaftLength * 0.5f, ScaledY, ScaledZ, v.r,
                        v.g, v.b, v.a, v.u, v.v, v.nx, v.ny, v.nz});
  }

  for (const auto &Index : ColoredCubeIndices) {
    Indices.push_back(Index);
  }

  for (const auto &v : ColoredCubeVertices) {
    float ScaledX = v.x * TipSize;
    float ScaledY = v.y * TipSize;
    float ScaledZ = v.z * TipSize;
    Vertices.push_back({ScaledX + TipSize * 0.5f + ShaftLength, ScaledY,
                        ScaledZ, v.r, v.g, v.b, v.a, v.u, v.v, v.nx, v.ny,
                        v.nz});
  }

  for (const auto &Index : ColoredCubeIndices) {
    Indices.push_back(Index + 8u);
  }

  const FMeshDesc Desc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#SquareArrow"), CreateInternalMesh(Renderer, Desc));
  return Library.AllMeshMap[FName("#SquareArrow")] != nullptr;
}


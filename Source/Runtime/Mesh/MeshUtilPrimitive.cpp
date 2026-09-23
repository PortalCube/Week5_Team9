#include "MeshUtil.h"

#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Geometry/Sphere.h"
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

bool MeshUtil::CreateCubeMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  FMeshDesc MeshDesc{
      .VertexData = CubeVertices,
      .VertexDataSize = static_cast<uint32>(sizeof(CubeVertices)),
      .VertexStride = static_cast<uint32>(sizeof(FVertexData)),
      .VertexCount = static_cast<uint32>(std::size(CubeVertices)),
      .IndexData = CubeIndices,
      .IndexDataSize = static_cast<uint32>(sizeof(CubeIndices)),
      .IndexCount = static_cast<uint32>(std::size(CubeIndices)),
  };

  Library.RegisterMesh(FName("#Cube"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Cube")] != nullptr;
}

bool MeshUtil::CreateCylinderMesh(FRenderer &Renderer, FRenderResourceLibrary &Library,
                                                float Height, uint32 SliceCount,
                                                float TopRadius,
                                                float BottomRadius) {
  constexpr float TAU = std::numbers::pi_v<float> * 2.0f;
  const float DTheta = TAU / static_cast<float>(SliceCount);

  TArray<FVertexData> Vertices;
  TArray<uint32> Indices;

  Vertices.reserve(SliceCount * 4 + 2);
  Indices.reserve(SliceCount * 12);

  const float HalfH = Height * 0.5f;

  const uint32 TopCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, HalfH, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f,
                      0.0f, 0.0f, 1.0f});

  const uint32 TopRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({TopRadius * std::cos(Theta),
                        TopRadius * std::sin(Theta), HalfH, 0.0f, 0.0f, 1.0f,
                        1.0f, 0.5f + 0.5f * std::cos(Theta),
                        0.5f + 0.5f * std::sin(Theta), 0.0f, 0.0f, 1.0f});
  }

  const uint32 BottomCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, -HalfH, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f,
                      0.0f, 0.0f, -1.0f});

  const uint32 BottomRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.5f + 0.5f * std::cos(Theta),
                        0.5f + 0.5f * std::sin(Theta), 0.0f, 0.0f, -1.0f});
  }

  const uint32 SideTopStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({TopRadius * std::cos(Theta),
                        TopRadius * std::sin(Theta), HalfH, 0.0f, 0.0f, 1.0f,
                        1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        0.0f, std::cos(Theta), std::sin(Theta), 0.0f});
  }

  const uint32 SideBottomStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 0.0f, 0.0f,
                        1.0f, 1.0f,
                        static_cast<float>(i) / static_cast<float>(SliceCount),
                        1.0f, std::cos(Theta), std::sin(Theta), 0.0f});
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(TopCenterIndex);
    Indices.push_back(TopRingStart + i);
    Indices.push_back(TopRingStart + Next);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(BottomCenterIndex);
    Indices.push_back(BottomRingStart + Next);
    Indices.push_back(BottomRingStart + i);
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;

    const uint32 TL = SideTopStart + i;
    const uint32 TR = SideTopStart + Next;
    const uint32 BL = SideBottomStart + i;
    const uint32 BR = SideBottomStart + Next;

    Indices.push_back(BL);
    Indices.push_back(BR);
    Indices.push_back(TL);

    Indices.push_back(BR);
    Indices.push_back(TR);
    Indices.push_back(TL);
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

  Library.RegisterMesh(FName("#Cylinder"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Cylinder")] != nullptr;
}

bool MeshUtil::CreateConeMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
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

  // 옆면 정점 생성
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

  // 밑면 뚜껑 정점 생성
  const uint32 BottomCenterIndex = static_cast<uint32>(Vertices.size());
  Vertices.push_back({0.0f, 0.0f, -HalfH, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f, 0.2f,
                      0.0f, 0.0f, -1.0f});

  const uint32 BottomRingStart = static_cast<uint32>(Vertices.size());
  for (uint32 i = 0; i < SliceCount; ++i) {
    const float Theta = static_cast<float>(i) * DTheta;
    const float U = static_cast<float>(i) / static_cast<float>(SliceCount);
    Vertices.push_back({BottomRadius * std::cos(Theta),
                        BottomRadius * std::sin(Theta), -HalfH, 1.0f, 1.0f,
                        1.0f, 1.0f, U, 1.0f, 0.0f, 0.0f, -1.0f});
  }

  for (uint32 i = 0; i < SliceCount; ++i) {
    const uint32 Next = (i + 1) % SliceCount;
    Indices.push_back(BottomCenterIndex);
    Indices.push_back(BottomRingStart + Next);
    Indices.push_back(BottomRingStart + i);
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

  Library.RegisterMesh(FName("#Cone"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Cone")] != nullptr;
}

bool MeshUtil::CreateSphereMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  auto Vertices = CreateSphereVertices(0.5f, 20, 20, false);

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(Vertices.size()),
  };

  Library.RegisterMesh(FName("#Sphere"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Sphere")] != nullptr;
}

bool MeshUtil::CreatePlaneMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  FMeshDesc Desc{
      .VertexData = PlaneVertices,
      .VertexDataSize = static_cast<uint32>(sizeof(PlaneVertices)),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(std::size(PlaneVertices)),
  };

  Library.RegisterMesh(FName("#Plane"), CreateInternalMesh(Renderer, Desc));
  return Library.AllMeshMap[FName("#Plane")] != nullptr;
}

bool MeshUtil::CreateRectMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  // 사각형 정점 배열
  const TArray<FVertexData> Vertices = {
      {0.0f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f},
      {0.0f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f},
      {0.0f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f},
      {0.0f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
       0.0f},
  };

  // -X 방향을 앞면으로 하는 인덱스 배열
  const TArray<uint32> Indices = {0, 1, 2, 0, 2, 3};

  FMeshDesc MeshDesc{
      .VertexData = Vertices.data(),
      .VertexDataSize =
          static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
      .VertexStride = sizeof(FVertexData),
      .VertexCount = static_cast<uint32>(Vertices.size()),
      .IndexData = Indices.data(),
      .IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
      .IndexCount = static_cast<uint32>(Indices.size()),
  };

  Library.RegisterMesh(FName("#Rect"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Rect")] != nullptr;
}


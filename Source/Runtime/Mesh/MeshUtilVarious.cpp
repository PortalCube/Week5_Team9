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

bool MeshUtil::CreateGridMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  constexpr float HalfW = 10.0f;
  constexpr float HalfH = 10.0f;

  const TArray<FVertexData> Vertices = {
      {-HalfW, -HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
       1.0f},
      {HalfW, -HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
       1.0f},
      {HalfW, HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
       1.0f},
      {-HalfW, HalfH, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
       1.0f},
  };
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

  Library.RegisterMesh(FName("#Grid"), CreateInternalMesh(Renderer, MeshDesc));
  return Library.AllMeshMap[FName("#Grid")] != nullptr;
}

bool MeshUtil::CreateLineMesh(FRenderer &Renderer, FRenderResourceLibrary &Library) {
  FMeshDesc Desc{.VertexData = LineVertices,
                 .VertexDataSize = static_cast<uint32>(sizeof(LineVertices)),
                 .VertexStride = sizeof(FVertexData),
                 .VertexCount = static_cast<uint32>(std::size(LineVertices)),
                 .bIsLine = true};

  Library.RegisterMesh(FName("#Line"), CreateInternalMesh(Renderer, Desc));
  return Library.AllMeshMap[FName("#Line")] != nullptr;
}


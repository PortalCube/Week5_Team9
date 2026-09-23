#pragma once

#include "FFont.h"
#include "FInstanceBatchKey.h"
#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Vertices.h"

class FRenderer;
class FTexture;
struct FTextVertex {
  FVector Pos;
  float u, v;
};

class FRenderResourceLibrary final {
public:
  // 전역 싱글톤 접근자
  static FRenderResourceLibrary &Get();

  bool Initialize(FRenderer &Renderer);

  // 파이프라인 보관 맵
  TMap<FName, TSharedPtr<FRenderPipeline>> AllPipelineMap;
  // 메쉬 보관 맵
  TMap<FName, TSharedPtr<FMesh>> AllMeshMap;
  // 머티리얼 보관 맵 (FName 기반)
  TMap<FName, TSharedPtr<FMaterial>> AllMaterialMap;
  // 텍스쳐 보관 맵 (FName 기반)
  TMap<FName, TSharedPtr<FTexture>> AllTextureMap;
  // 폰트 보관 맵
  TMap<FName, TSharedPtr<FFont>> AllFontMap;

  // 인스턴싱 배치 배열 맵
  TMap<FInstanceBatchKey, TArray<FInstanceData>> AllInstancingArrayMap;

  // 인스턴싱 배열 조회
  TArray<FInstanceData>& GetInstancingArray(const FMesh* Mesh, const FMaterial* Material) {
    return AllInstancingArrayMap[{Mesh, Material}];
  }

  // 파이프라인 조회
  [[nodiscard]] TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const {
    auto it = AllPipelineMap.find(Id);
    if (it != AllPipelineMap.end())
      return it->second;
    return nullptr;
  }

  void RegisterPipeline(const FName& Id, TSharedPtr<FRenderPipeline> Pipeline) {
    AllPipelineMap[Id] = std::move(Pipeline);
  }

  // 머티리얼 조회
  [[nodiscard]] TSharedPtr<FMaterial> GetMaterial(const FName& Id) const {
    auto it = AllMaterialMap.find(Id);
    if (it != AllMaterialMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 조회
  TSharedPtr<FMesh> GetMesh(const FName &ID) const {
    auto it = AllMeshMap.find(ID);
    if (it != AllMeshMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 등록
  TSharedPtr<FMesh> RegisterMesh(const FName &ID, TSharedPtr<FMesh> inMesh) {
    inMesh->MeshId = ID;
    AllMeshMap[ID] = inMesh;
    return inMesh;
  }

  // 머티리얼 등록
  TSharedPtr<FMaterial> RegisterMaterial(const FName& Id, TSharedPtr<FMaterial> inMaterial);

  void RegisterTexture(const FName &name, TSharedPtr<FTexture> texture) {
    AllTextureMap[name] = texture;
  }

  // 텍스처 조회
  [[nodiscard]] TSharedPtr<FTexture> GetTexture(const FName &name) const {
    auto it = AllTextureMap.find(name);
    if (it != AllTextureMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 전체 해제
  void DestroyAllMeshes() { AllMeshMap.clear(); }

  // 머티리얼 전체 해제
  void DestroyAllMaterials() { AllMaterialMap.clear(); }

  // 파이프라인 전체 해제
  void DestroyAllPipelines() { AllPipelineMap.clear(); }

  void DestroyAllInstancingArray() { AllInstancingArrayMap.clear(); }

  // 전체 머티리얼 맵 조회
  const TMap<FName, TSharedPtr<FMaterial>> &GetAllMaterials() const {
    return AllMaterialMap;
  }

  const TMap<FName, TSharedPtr<FRenderPipeline>>& GetAllPipelines() const {
    return AllPipelineMap;
  }

  const TMap<FName, TSharedPtr<FTexture>>& GetAllTextures() const {
    return AllTextureMap;
  }

  // 렌더러 참조 조회
  FRenderer *GetRenderer() const { return RendererRef; }

  // 정점 배열 메쉬 캐싱 생성
  TSharedPtr<FMesh> GetOrCreateMesh(const FName &ID,
                                    const TArray<FVertexData> &vertices);

  [[nodiscard]] TSharedPtr<FFont> GetFont(const FName& InName) const {
      auto it = AllFontMap.find(InName);
      if (it != AllFontMap.end())
          return it->second;
      return nullptr;
  }

private:
  bool InitializePipelines(FRenderer &Renderer);
  bool CreateWireframePipeline(FRenderer &Renderer);
  bool CreateOutlinePipeline(FRenderer &Renderer);
  bool CreatePostProcessPipeline(FRenderer &Renderer);

  bool CreateInstancingArrayMap();
  FRenderer *RendererRef = nullptr;
};

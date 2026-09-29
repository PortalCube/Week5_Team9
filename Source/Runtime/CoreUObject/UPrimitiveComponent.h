#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "USceneComponent.h"
#include "Runtime/Engine/FCulling.h"
#include "Runtime/Rendering/FMaterial.h"

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual void SetMesh(UStaticMesh* Mesh);

    void SetMaterial(UMaterial* Material, int32 Index = 0);
    void SetTexture(UTexture* Texture, int32 Index = 0);
    void SetRenderType(ERenderType Type) { RenderData.Type = Type; }
    void SetColor(const FVector4& Color, int32 Index = 0);

    virtual void SetRelativeTransform(const FTransform& RelativeTransform) override;

    virtual const FRenderData& GetRenderData(const FCamera& Camera) const { return RenderData; }
    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().GetMatrix(); }

    virtual FAxisAlignedBoundingBox GetLocalBounds() const { return LocalBounds; }
    virtual FAxisAlignedBoundingBox GetWorldBounds() const;
    virtual FAxisAlignedBoundingBox GetViewBounds(const FCamera& Camera) const;
    const UStaticMesh* GetMeshAsset() const { return RenderData.Mesh; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }
    int32 GetBVHIndex() const { return BVHIndex; }
    void SetBVHIndex(int32 i) { BVHIndex = i; }

    void MarkBoundDirty();
    int32 GetSceneIndex() const { return SceneIndex; }
    void SetSceneIndex(int32 pIndex) { SceneIndex = pIndex; }

    bool GetBoundDirtyQueued()const { return bBoundDirtyQueued; }
    void SetBoundDirtyQueued(bool pDirtyQueued) { bBoundDirtyQueued = pDirtyQueued; }

    //월드 AABB 업데이트
    void UpdateWorldBounds();

    const std::vector<FMaterial>& GetCachedMaterials() const { return CachedMaterials; }
    void UpdateMaterialCache();

protected:
    UPrimitiveComponent() = default;

    mutable FRenderData RenderData
    {
       .Mesh = nullptr,
       .Materials = {},
       .ModelMatrix = FMatrix::Identity,
       .Type = ERenderType::None,
    };

    FAxisAlignedBoundingBox LocalBounds{};
    FAxisAlignedBoundingBox WorldBounds{};

    FVector Color{1.0f, 1.0f, 1.0f};
    float ColorAmount = 0.0f;

    //FSceneBVH 내의 역질의용 index, -1면 BVH에 없음
    int32 BVHIndex = -1;

    void OnTransformChanged() override;

private:
    int32 SceneIndex = -1;
    bool bBoundDirtyQueued = false;

    TArray<FMaterial> CachedMaterials;
};

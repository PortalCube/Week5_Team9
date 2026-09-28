#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "USceneComponent.h"
#include "Runtime/Engine/FCulling.h"

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

    virtual const FRenderData& GetRenderData(const FCamera& Camera) const { return RenderData; }
    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().ToMatrix(); }

    virtual FAxisAlignedBoundingBox GetLocalBounds() const { return {}; }
    const UStaticMesh* GetMeshAsset() const { return RenderData.Mesh; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }
    int32 GetBVHIndex() const { return BVHIndex; }
    void SetBVHIndex(int32 i) { BVHIndex = i; }

    //컬링용 월드 AABB. 나중에 다른 브랜치의 월드 AABB가 생기면 합칠 수 있다.
    FCullData CalcWorldCullData() const;
    //월드 행렬이 카메라에 의존적인지 : 빌보드 같은 건 true로 override한다.
    virtual bool HasCameraDependentTransform() const { return false; }
    void MarkBoundDirty();
    int32 GetSceneIndex() const { return SceneIndex; }
    void SetSceneIndex(int32 pIndex) { SceneIndex = pIndex; }

    bool GetBoundDirtyQueued()const { return bBoundDirtyQueued; }
    void SetBoundDirtyQueued(bool pDirtyQueued) { bBoundDirtyQueued = pDirtyQueued; }

protected:
    UPrimitiveComponent() = default;

    mutable FRenderData RenderData
    {
       .Mesh = nullptr,
       .Materials = {},
       .ModelMatrix = FMatrix::Identity,
       .Type = ERenderType::None,
    };

    FVector Color{1.0f, 1.0f, 1.0f};
    float ColorAmount = 0.0f;

    //FSceneBVH 내의 역질의용 index, -1면 BVH에 없음
    int32 BVHIndex = -1;

    void OnTransformChanged() override;

private:
    int32 SceneIndex = -1;
    bool bBoundDirtyQueued = false;
};

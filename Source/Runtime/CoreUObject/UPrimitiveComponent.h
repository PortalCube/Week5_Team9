#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UPrimitiveComponent, USceneComponent)

public:
    void Initialize() override;
    void Register(UScene& InScene) override;
    void Unregister() override;

    virtual void SetMesh(UStaticMesh* Mesh) { RenderData.Mesh = Mesh; }
    void SetMaterial(UMaterial* Material, int32 Index = 0);
    void SetTexture(UTexture* Texture, int32 Index = 0);
    void SetRenderType(ERenderType Type) { RenderData.Type = Type; }
    void SetColor(const FVector4& Color, int32 Index = 0);

    virtual const FRenderData& GetRenderData(const FCamera& Camera) const { return RenderData; }
    virtual FMatrix GetRenderMatrix(const FCamera& Camera) const { return GetGlobalTransform().ToMatrix(); }

    virtual FAxisAlignedBoundingBox CalcLocalBounds() { return {}; }

    virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_Primitives; }

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
};

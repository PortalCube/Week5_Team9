#pragma once


#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"

class UStaticMesh;
class UMaterial;
struct FMaterialInstance;

/// <summary>
/// 모든 Mesh Component의 부모 컴포넌트입니다.
/// </summary>
class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UMeshComponent, UPrimitiveComponent)

public:

    // 의도적으로 nullptr 반환
    virtual const UStaticMesh* GetMesh() { return nullptr; }
    virtual const UMaterial* GetMaterial(int Index = 0) const { return nullptr; }
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const { return nullptr; }
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const { return nullptr; }

    virtual EEngineShowFlags GetShowFlag() const override { return EEngineShowFlags::SF_Primitives; }

protected:
    UMeshComponent() = default;
};

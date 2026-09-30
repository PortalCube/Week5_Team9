
#pragma once

#include "Runtime/CoreUObject/Mesh/UMeshComponent.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Material/FMaterialInstance.h"
#include <Runtime/Geometry/FAxisAlignedBoundingBox.h>

class UStaticMeshComponent : public UMeshComponent {
    GENERATED_BODY()
    DECLARE_UCLASS(UStaticMeshComponent, UMeshComponent)

public:
    virtual void SetMesh(UStaticMesh* Mesh) override;

    virtual const UStaticMesh* GetMesh() override { return RenderData.Mesh; }
    virtual const UMaterial* GetMaterial(int Index = 0) const override;
    virtual const FMaterialInstance* GetMaterialInstance(int Index = 0) const override;
    virtual const TArray<FMaterialInstance>* GetAllMaterialInstance() const override { return &RenderData.Materials; }

    void SetMaterialInstance(const FMaterialInstance& Instance, int Index = 0);
    void SetPipeline(UPipeline* Pipeline, int Index = 0);
    void SetTexture(UTexture* Texture, int Index = 0);

    virtual FAxisAlignedBoundingBox GetLocalBounds() const override;

    void ClearMaterial();

    int32 GetMaterialSlotLength() const;

    virtual EEngineShowFlags GetShowFlag() const;

    // 바운딩 구의 지름이 화면 높이의 몇 배를 차지하는지 계산한다.
    float ComputeScreenSize(const FCamera& Camera) const;
    // ComputeScreenSize의 제곱. 제곱근이 없어 LOD 선택처럼 매 프레임 도는 곳에서 쓴다.
    float ComputeScreenSizeSquared(const FCamera& Camera) const;
    uint32 SelectLOD(const FCamera& Camera) const;

    // 컴포넌트를 읽지 않고 바운드만으로 계산하는 버전. 렌더링 수집에서 씬의 연속 배열(CullDataList)의
    // 바운드를 넘겨, 컴포넌트마다 WorldBounds를 읽는 캐시 미스를 피한다.
    static float ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& WorldBounds, const FCamera& Camera);
    static uint32 SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FCamera& Camera);

    // LOD 계산에 쓰는 카메라 값. 모든 오브젝트에서 같으므로 프레임(뷰)당 한 번만 만들어 넘긴다.
    // 오브젝트마다 카메라 getter와 std::max를 호출하는 비용을 없애기 위함.
    struct FLODView
    {
        float CamX = 0.0f, CamY = 0.0f, CamZ = 0.0f;
        float MultipleSq = 1.0f;      // 원근: (1/tan(FOV/2))^2
        float OrthoHeightSq = 1.0f;   // 직교: max(높이, 1e-4)^2
        bool bOrthographic = false;
    };
    static FLODView MakeLODView(const FCamera& Camera);
    static float ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& WorldBounds, const FLODView& View);
    static uint32 SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FLODView& View);

protected:
    UStaticMeshComponent() = default;

    virtual void Serialize(FArchive& Archive) const override;
    virtual void Deserialize(const FArchive& Archive) override;
};

#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Rendering/Vertices.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "UPrimitiveComponent.h"
#include <d3d11.h>
#include <wrl/client.h>

class UInstancePrimitiveComponent : public UPrimitiveComponent {
  GENERATED_BODY()
  DECLARE_UCLASS(UInstancePrimitiveComponent, UPrimitiveComponent)

public:
    void Initialize() override;

    // 큐 방식: FRenderData에 Instances까지 채워서 반환
    virtual void BuildRenderData() const;
    virtual const FRenderData& GetRenderData(const FCamera& Camera) const override;

    // 인스턴스 위치/색상 추가 (Actor 1개가 N개 위치를 직접 관리)
    void AddInstance(const FVector& WorldPosition, const FVector4& Color = {1,1,1,1});
    void ClearInstances();
    int32 GetInstanceCount() const { return static_cast<int32>(InstanceTransforms.size()); }

private:
    struct FInstanceEntry { FVector Position; FVector4 Color; };
    TArray<FInstanceEntry> InstanceTransforms;
};

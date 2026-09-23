#include "UInstancePrimitiveComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Engine/UScene.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UInstancePrimitiveComponent, UPrimitiveComponent)

void UInstancePrimitiveComponent::Initialize()
{
    Super::Initialize();

    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    //SetMaterial(Registry.Get<UMaterial>("Material/Instance_Textured.json"));
    
    RenderData.Type = ERenderType::Instancing;

}

void UInstancePrimitiveComponent::AddInstance(const FVector& WorldPosition, const FVector4& Color)
{
    InstanceTransforms.push_back({ WorldPosition, Color });
}

void UInstancePrimitiveComponent::ClearInstances()
{
    InstanceTransforms.clear();
}

void UInstancePrimitiveComponent::BuildRenderData() const
{
    TArray<FInstanceData> Built;
    const FTransform BaseTransform = GetGlobalTransform();

    if (InstanceTransforms.empty())
    {
        // 등록된 인스턴스 없으면 자기 자신 트랜스폼 적용
        Built.push_back(FInstanceData{
            .World    = BaseTransform.ToMatrix(),
            .Color    = FVector4(1.0f, 1.0f, 1.0f, 1.0f),
            .UVScale  = {1.0f, 1.0f},
            .UVOffset = {0.0f, 0.0f},
        });
    }
    else
    {
        Built.reserve(InstanceTransforms.size());
        for (const auto& Entry : InstanceTransforms)
        {
            FTransform InstTransform = BaseTransform;
            InstTransform.Location = Entry.Position;
            Built.push_back(FInstanceData{
                .World    = InstTransform.ToMatrix(),
                .Color    = Entry.Color,
                .UVScale  = {1.0f, 1.0f},
                .UVOffset = {0.0f, 0.0f},
            });
        }
    }

    RenderData.Instances = std::move(Built);
}

const FRenderData& UInstancePrimitiveComponent::GetRenderData(const FCamera& Camera) const
{
    BuildRenderData();
    return RenderData;
}

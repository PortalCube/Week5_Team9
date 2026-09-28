#include "UPrimitiveComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Asset/UStaticMesh.h"

IMPLEMENT_UCLASS(UPrimitiveComponent, USceneComponent)

void UPrimitiveComponent::Initialize()
{
    Super::Initialize();
    RenderData.Type = ERenderType::Primitive;

    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    FMaterialInstance DefaultMaterial{ Registry.Get<UMaterial>("Material/Simple.json") };

    RenderData.Materials.push_back(DefaultMaterial);
}

void UPrimitiveComponent::SetMesh(UStaticMesh* Mesh)
{
    RenderData.Mesh = Mesh;
    MarkBoundDirty();
}

void UPrimitiveComponent::SetMaterial(UMaterial* Material, int32 Index)
{
    if (!Material || Index < 0) { return; }

    const size_t TargetIndex = static_cast<size_t>(Index);
    if (RenderData.Materials.size() <= TargetIndex)
    {
        RenderData.Materials.resize(TargetIndex + 1, FMaterialInstance{ Material });
    }
    RenderData.Materials[TargetIndex] = FMaterialInstance{ Material };
}

void UPrimitiveComponent::SetTexture(UTexture* Texture, int32 Index)
{
    if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
    RenderData.Materials[static_cast<size_t>(Index)].Texture = Texture;
}

void UPrimitiveComponent::SetColor(const FVector4& Color, int32 Index)
{
    if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
    RenderData.Materials[static_cast<size_t>(Index)].Color = Color;
}

FCullData UPrimitiveComponent::CalcWorldCullData() const
{
    //빌보드처럼 카메라에 의존적이라면 항상 포함하도록 합니다.
    if (HasCameraDependentTransform())
    {
        return MakeAlwaysVisibleCullData();
    }

    //메시 가져오기
    const FMesh* Mesh = RenderData.Mesh ? RenderData.Mesh->Get() : nullptr;

    if (!Mesh)
    {
        return MakeAlwaysVisibleCullData();
    }

    const FAxisAlignedBoundingBox& Local = Mesh->GetLocalBounds();
    if (Local.Min.X > Local.Max.X)
    {
        return MakeAlwaysVisibleCullData();
    }

    const FVector LocalCenter = (Local.Min + Local.Max) * 0.5f;
    const FVector LocalExtent = (Local.Max - Local.Min) * 0.5f;

    const FMatrix W = GetGlobalTransform().ToMatrix();

    FCullData Out;

    Out.Center.X = LocalCenter.X * W.M[0][0] + LocalCenter.Y * W.M[1][0] + LocalCenter.Z * W.M[2][0] + W.M[3][0];
    Out.Center.Y = LocalCenter.X * W.M[0][1] + LocalCenter.Y * W.M[1][1] + LocalCenter.Z * W.M[2][1] + W.M[3][1];
    Out.Center.Z = LocalCenter.X * W.M[0][2] + LocalCenter.Y * W.M[1][2] + LocalCenter.Z * W.M[2][2] + W.M[3][2];

    Out.Extent.X = LocalExtent.X * std::fabs(W.M[0][0]) + LocalExtent.Y * std::fabs(W.M[1][0]) + LocalExtent.Z * std::fabs(W.M[2][0]);
    Out.Extent.Y = LocalExtent.X * std::fabs(W.M[0][1]) + LocalExtent.Y * std::fabs(W.M[1][1]) + LocalExtent.Z * std::fabs(W.M[2][1]);
    Out.Extent.Z = LocalExtent.X * std::fabs(W.M[0][2]) + LocalExtent.Y * std::fabs(W.M[1][2]) + LocalExtent.Z * std::fabs(W.M[2][2]);

    return Out;
}

void UPrimitiveComponent::MarkBoundDirty()
{
    if (Scene)
    {
        Scene->MarkBoundsDirty(this);
    }
}

void UPrimitiveComponent::OnTransformChanged()
{
    MarkBoundDirty();
}

void UPrimitiveComponent::Register(UScene& InScene)
{
    if (RenderData.Type == ERenderType::None)
    {
        RenderData.Type = (RenderData.Materials.size() > 0 && RenderData.Materials[0].Texture)
            ? ERenderType::Texture
            : ERenderType::Primitive;
    }

    Super::Register(InScene);
    InScene.AddRenderComponent(this);
}

void UPrimitiveComponent::Unregister()
{
    if (Scene)
    {
        Scene->RemoveRenderComponent(this);
    }
    Super::Unregister();
}

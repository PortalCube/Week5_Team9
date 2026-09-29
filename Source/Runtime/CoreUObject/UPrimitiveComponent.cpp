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
    UpdateMaterialCache();
}

void UPrimitiveComponent::SetMesh(UStaticMesh* Mesh)
{
    RenderData.Mesh = Mesh;
    LocalBounds = Mesh->Get()->GetLocalBounds();
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
    UpdateMaterialCache();
}

void UPrimitiveComponent::SetTexture(UTexture* Texture, int32 Index)
{
    if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
    RenderData.Materials[static_cast<size_t>(Index)].Texture = Texture;
    UpdateMaterialCache();
}

void UPrimitiveComponent::SetColor(const FVector4& Color, int32 Index)
{
    if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
    RenderData.Materials[static_cast<size_t>(Index)].Color = Color;
}

void UPrimitiveComponent::MarkBoundDirty()
{
    if (Scene)
    {
        Scene->MarkBoundsDirty(this);
    }
}

void UPrimitiveComponent::UpdateWorldBounds()
{
    WorldBounds = FAxisAlignedBoundingBox(GetLocalBounds(), GetGlobalTransformMatrix());
}

void UPrimitiveComponent::OnTransformChanged()
{
    MarkBoundDirty();
}

void UPrimitiveComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
    Super::SetRelativeTransform(RelativeTransform);
    UpdateWorldBounds();
}

const FAxisAlignedBoundingBox &UPrimitiveComponent::GetWorldBounds() const
{
    return WorldBounds;
}

FAxisAlignedBoundingBox UPrimitiveComponent::GetViewBounds(const FCamera& Camera) const
{
    return FAxisAlignedBoundingBox(GetWorldBounds(), Camera.GetViewMatrix());
}

void UPrimitiveComponent::Register(UScene& InScene)
{
    if (RenderData.Type == ERenderType::None)
    {
        RenderData.Type = ERenderType::Primitive;
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

void UPrimitiveComponent::UpdateMaterialCache()
{
	CachedMaterials.clear();
	CachedMaterials.reserve(RenderData.Materials.size());
	
	for (const auto& Item : RenderData.Materials)
	{
		if (!Item.Pipeline)
		{
			continue;
		}

		FMaterial Material{};
		Material.SetPipeLine(Item.Pipeline->Get());

		if (Item.Texture)
		{
			Material.SetTexture(Item.Texture->Get());
		}
		Material.SetSamplerDesc(Item.SamplerDesc);
		CachedMaterials.push_back(Material);
	}
}
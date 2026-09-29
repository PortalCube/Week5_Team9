#include "UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Core/Globals.h"
#include <numbers>


IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

const FRenderData& UStaticMeshComponent::GetRenderData(const FCamera& Camera) const
{
	//RenderData.ModelMatrix = GetRenderMatrix(Camera);
	RenderData.LODIndex = SelectLOD(Camera);
	return RenderData;
}

float UStaticMeshComponent::ComputeScreenSize(const FCamera& Camera) const
{
	return std::sqrt(ComputeScreenSizeSquared(Camera));
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FCamera& Camera) const
{
	const FAxisAlignedBoundingBox Bounds = GetWorldBounds();
	if (!Bounds.IsValid()) { return 1.0f; }

	// 바운딩 박스를 감싸는 구로 근사한다.
	const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;
	const FVector HalfExtent = (Bounds.Max - Bounds.Min) * 0.5f;
	const float RadiusSq = HalfExtent.X * HalfExtent.X + HalfExtent.Y * HalfExtent.Y + HalfExtent.Z * HalfExtent.Z;

	const FCameraProjection& Projection = Camera.GetProjection();
	if (Projection.GetProjectionType() == EProjectionType::Orthographic)
	{
		const float Height = std::max(Projection.GetOrthographicHeight(), 1e-4f);
		return 4.0f * RadiusSq / (Height * Height);
	}

	// 언리얼의 ComputeBoundsScreenSize와 같은 방식. 구의 지름이 화면 높이의 몇 배인지의 제곱을 반환한다.
	// 배율(1/tan(FOV/2))은 투영이 바뀔 때 FCameraProjection에서 한 번만 계산해 둔다.
	const FVector Offset = Center - Camera.GetPosition();
	const float DistanceSq = std::max(Offset.X * Offset.X + Offset.Y * Offset.Y + Offset.Z * Offset.Z, 1e-8f);
	const float Multiple = Projection.GetScreenSizeMultiple();
	return Multiple * Multiple * RadiusSq / DistanceSq;
}

uint32 UStaticMeshComponent::SelectLOD(const FCamera& Camera) const
{
	const UStaticMesh* Mesh = RenderData.Mesh;
	if (!Mesh || Mesh->GetLODCount() <= 1 || !Globals::bEnableLOD) { return 0; }

	if (Globals::ForcedLOD >= 0)
	{
		return std::min(static_cast<uint32>(Globals::ForcedLOD), Mesh->GetLODCount() - 1);
	}

	return Mesh->SelectLODSquared(ComputeScreenSizeSquared(Camera));
}

FAxisAlignedBoundingBox UStaticMeshComponent::GetLocalBounds() const
{
	const FMesh* Mesh = RenderData.Mesh ? RenderData.Mesh->Get() : nullptr;
	return Mesh ? Mesh->GetLocalBounds() : FAxisAlignedBoundingBox{};
}

void UStaticMeshComponent::SetMesh(UStaticMesh* Mesh)
{
	UPrimitiveComponent::SetMesh(Mesh);

	const TArray<FMeshSection>& Sections = Mesh->Get()->GetSections();
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	UMaterial* FallbackMaterial = Registry.Get<UMaterial>("Material/Simple.json");

	RenderData.Materials.clear();
	for (size_t i = 0; i < Sections.size(); i++)
	{
		UMaterial* Mat = Registry.Get<UMaterial>(FName(Sections[i].SectionName));
		SetMaterial(Mat ? Mat : FallbackMaterial, static_cast<int32>(i));
	}

	// 메시에서 유효한 Material 정보를 하나도 찾지 못하면 기본 Material을 사용한다.
	if (RenderData.Materials.empty())
	{
		SetMaterial(FallbackMaterial, 0);
	}
}

const UMaterial* UStaticMeshComponent::GetMaterial(int Index) const
{
	const FMaterialInstance* Instance = GetMaterialInstance(Index);
	return Instance ? Instance->Material : nullptr;
}

const FMaterialInstance* UStaticMeshComponent::GetMaterialInstance(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return nullptr; }
	return &RenderData.Materials[static_cast<size_t>(Index)];
}

int32 UStaticMeshComponent::GetMaterialSlotLength() const
{
	if (RenderData.Mesh && RenderData.Mesh->Get())
	{
		// 잘못된 메시가 섹션 없이 들어와도 Material을 지정할 슬롯은 하나 제공한다.
		return std::max(1, static_cast<int32>(RenderData.Mesh->Get()->GetSectionCount()));
	}

	return static_cast<int32>(RenderData.Materials.size());
}

void UStaticMeshComponent::SetMaterialInstance(const FMaterialInstance& Instance, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
	RenderData.Materials[static_cast<size_t>(Index)] = Instance;
}

void UStaticMeshComponent::SetPipeline(UPipeline* Pipeline, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Pipeline = Pipeline;
}

void UStaticMeshComponent::SetTexture(UTexture* Texture, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Texture = Texture;
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
}

EEngineShowFlags UStaticMeshComponent::GetShowFlag() const
{
	return EEngineShowFlags::SF_Primitives;
}

void UStaticMeshComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	if (!RenderData.Mesh)
	{
		return;
	}

	FString MeshAssetID = RenderData.Mesh->GetID().ToString();
	TArray<FArchive> MaterialAsset = {};

	for (const auto& Item : *GetAllMaterialInstance())
	{
		FArchive ItemArchive{};
		ItemArchive.SetFloat("Albedo", Item.Albedo);
		ItemArchive.SetFloat("Diffuse", Item.Diffuse);
		ItemArchive.SetFloat("Specular", Item.Specular);

		FString MaterialID = "";
		if (Item.Material)
		{
			MaterialID = Item.Material->GetID().ToString();
		}
		ItemArchive.SetString("MaterialAsset", MaterialID);

		FString PipelineID = "";
		if (Item.Pipeline)
		{
			PipelineID = Item.Pipeline->GetID().ToString();
		}
		ItemArchive.SetString("OverridePipelineAsset", PipelineID);

		FString TextureID = "";
		if (Item.Texture)
		{
			TextureID = Item.Texture->GetID().ToString();
		}
		ItemArchive.SetString("OverrideTextureAsset", TextureID);

		ItemArchive.SetBool("DisableShading", Item.bDisableShading);

		ItemArchive.SetVector4("Color", Item.Color);
		ItemArchive.SetVector2("UVOffset", Item.UVOffset);
		ItemArchive.SetVector2("UVScale", Item.UVScale);

		MaterialAsset.push_back(ItemArchive);
	}

	Archive.SetString("MeshAsset", MeshAssetID);
	Archive.SetArchiveArray("Materials", MaterialAsset);
}

void UStaticMeshComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	if (Archive.IsNull("MeshAsset"))
	{
		return;
	}

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	FString MeshAssetID = Archive.GetString("MeshAsset");
	UStaticMesh* Mesh = Registry.Get<UStaticMesh>(MeshAssetID);

	if (!Mesh)
	{
		return;
	}

	SetMesh(Mesh);

	if (Archive.IsNull("Materials"))
	{
		return;
	}

	TArray<FArchive> MaterialArchives = Archive.GetArchiveArray("Materials");

	for (int i = 0; i < MaterialArchives.size(); ++i)
	{
		FArchive& Item = MaterialArchives[i];

		FName MaterialID = Item.GetString("MaterialAsset");
		UMaterial* Material = Registry.Get<UMaterial>(MaterialID);
		FMaterialInstance Instance{ Material };

		FString PipelineAssetID = Item.GetString("OverridePipelineAsset");
		UPipeline* Pipeline = Registry.Get<UPipeline>(PipelineAssetID);
		if (Pipeline)
		{
			Instance.Pipeline = Pipeline;
		}

		FString TextureAssetID = Item.GetString("OverrideTextureAsset");
		UTexture* Texture = Registry.Get<UTexture>(TextureAssetID);
		if (Texture)
		{
			Instance.Texture = Texture;
		}
		
		Instance.bDisableShading = Item.GetBool("DisableShading");

		Instance.Color = Item.GetVector4("Color");
		Instance.UVOffset = Item.GetVector2("UVOffset");
		Instance.UVScale = Item.GetVector2("UVScale");

		SetMaterialInstance(Instance, i);
	}
}


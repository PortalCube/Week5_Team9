#include "UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Core/Globals.h"
#include <numbers>


IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)

float UStaticMeshComponent::ComputeScreenSize(const FCamera& Camera) const
{
	return std::sqrt(ComputeScreenSizeSquared(Camera));
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FCamera& Camera) const
{
	return ComputeScreenSizeSquared(GetWorldBounds(), Camera);
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& Bounds, const FCamera& Camera)
{
	return ComputeScreenSizeSquared(Bounds, MakeLODView(Camera));
}

UStaticMeshComponent::FLODView UStaticMeshComponent::MakeLODView(const FCamera& Camera)
{
	FLODView View;
	const FVector& CameraPosition = Camera.GetPosition();
	View.CamX = CameraPosition.X;
	View.CamY = CameraPosition.Y;
	View.CamZ = CameraPosition.Z;

	// 배율(1/tan(FOV/2))은 투영이 바뀔 때 FCameraProjection에서 한 번만 계산해 둔다.
	const FCameraProjection& Projection = Camera.GetProjection();
	View.bOrthographic = Projection.GetProjectionType() == EProjectionType::Orthographic;
	const float Multiple = Projection.GetScreenSizeMultiple();
	View.MultipleSq = Multiple * Multiple;
	const float Height = std::max(Projection.GetOrthographicHeight(), 1e-4f);
	View.OrthoHeightSq = Height * Height;
	return View;
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& Bounds, const FLODView& View)
{
	// 매 프레임 오브젝트마다 도는 코드라, 함수 호출(IsValid, std::max, FVector 연산자) 없이 float로만 계산한다.
	// 월드 바운드의 Center/Extent는 바운드가 갱신될 때 이미 계산되어 있다.
	if (Bounds.Min.X > Bounds.Max.X || Bounds.Min.Y > Bounds.Max.Y || Bounds.Min.Z > Bounds.Max.Z) { return 1.0f; }

	// 바운딩 박스를 감싸는 구로 근사한다. 반지름 = Extent의 길이.
	const float EX = Bounds.Extent.X, EY = Bounds.Extent.Y, EZ = Bounds.Extent.Z;
	const float RadiusSq = EX * EX + EY * EY + EZ * EZ;

	if (View.bOrthographic)
	{
		return 4.0f * RadiusSq / View.OrthoHeightSq;
	}

	// 언리얼의 ComputeBoundsScreenSize와 같은 방식. 구의 지름이 화면 높이의 몇 배인지의 제곱을 반환한다.
	const float DX = Bounds.Center.X - View.CamX;
	const float DY = Bounds.Center.Y - View.CamY;
	const float DZ = Bounds.Center.Z - View.CamZ;
	float DistanceSq = DX * DX + DY * DY + DZ * DZ;
	DistanceSq = DistanceSq > 1e-8f ? DistanceSq : 1e-8f;
	return View.MultipleSq * RadiusSq / DistanceSq;
}

uint32 UStaticMeshComponent::SelectLOD(const FCamera& Camera) const
{
	return SelectLOD(RenderData.Mesh, GetWorldBounds(), Camera);
}

uint32 UStaticMeshComponent::SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FCamera& Camera)
{
	return SelectLOD(Mesh, WorldBounds, MakeLODView(Camera));
}

uint32 UStaticMeshComponent::SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FLODView& View)
{
	if (!Globals::bEnableLOD) { return 0; }
	if (!Mesh) { return 0; }

	const uint32 LODCount = Mesh->GetLODCount();
	if (LODCount <= 1) { return 0; }

	if (Globals::ForcedLOD >= 0)
	{
		const uint32 Forced = static_cast<uint32>(Globals::ForcedLOD);
		return Forced < LODCount - 1 ? Forced : LODCount - 1;
	}

	return Mesh->SelectLODSquared(ComputeScreenSizeSquared(WorldBounds, View));
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
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::SetPipeline(UPipeline* Pipeline, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Pipeline = Pipeline;
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::SetTexture(UTexture* Texture, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size()) { return; }
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Texture = Texture;
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
	UpdateMaterialCache();
	UpdateSortKey();
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


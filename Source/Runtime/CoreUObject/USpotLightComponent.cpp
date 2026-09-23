#include "USpotLightComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "UClass.h"

IMPLEMENT_UCLASS(USpotLightComponent, UPrimitiveComponent)
UCLASS_META(USpotLightComponent, DisplayName, "SpotLight")
UCLASS_META(USpotLightComponent, MeshName, "#SpotlightCone")

void USpotLightComponent::Initialize()
{
	Super::Initialize();
	// 스포트라이트 메쉬 및 머티리얼 장착
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	SetMesh(Registry.Get<UStaticMesh>("#SpotlightCone"));
	SetMaterial(Registry.Get<UMaterial>("Material/Spotlight.json"));
	RenderData.Type = ERenderType::Spotlight;
}

void USpotLightComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetFloat("SpotAngle", SpotAngle);
	Archive.SetFloat("Range", Range);
	Archive.SetFloat("Intensity", Intensity);
	Archive.SetVector("LightColor", LightColor);
}

void USpotLightComponent::Deserialize(const FArchive & Archive)
{
	Super::Deserialize(Archive);

	SpotAngle = Archive.GetFloat("SpotAngle");
	Range = Archive.GetFloat("Range");
	Intensity = Archive.GetFloat("Intensity");
	LightColor = Archive.GetVector("LightColor");

}

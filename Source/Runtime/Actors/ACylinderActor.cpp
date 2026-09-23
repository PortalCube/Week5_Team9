#include "ACylinderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACylinderActor, AActor)
UCLASS_META(ACylinderActor, DisplayName, "Cylinder Actor")

ACylinderActor::ACylinderActor()
{
	// 기본 실린더 컴포넌트 장착
	UStaticMeshComponent* Object = NewObject<UStaticMeshComponent>();
	SetRootComponent(Object);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	Object->SetMesh(Registry.Get<UStaticMesh>("#Cylinder"));
	Object->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
}

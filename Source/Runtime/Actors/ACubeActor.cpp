#include "ACubeActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACubeActor, AActor)
UCLASS_META(ACubeActor, DisplayName, "Cube Actor")

ACubeActor::ACubeActor()
{
	// 기본 큐브 컴포넌트 장착
	UStaticMeshComponent* Object = NewObject<UStaticMeshComponent>();
	SetRootComponent(Object);
	
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	Object->SetMesh(Registry.Get<UStaticMesh>("#Cube"));
	Object->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
}

#include "ASphereActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ASphereActor, AActor)
UCLASS_META(ASphereActor, DisplayName, "Sphere Actor")

ASphereActor::ASphereActor()
{
	// 기본 구체 컴포넌트 장착
	UStaticMeshComponent* Object = NewObject<UStaticMeshComponent>();
	SetRootComponent(Object);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	Object->SetMesh(Registry.Get<UStaticMesh>("#Sphere"));
	Object->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
}

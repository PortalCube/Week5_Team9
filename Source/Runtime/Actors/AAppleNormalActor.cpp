#include "AAppleNormalActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleNormalActor, AActor)
UCLASS_META(AAppleNormalActor, DisplayName, "Apple Normal Actor")

AAppleNormalActor::AAppleNormalActor()
{	
	AppleStaticMeshComp = NewObject<UStaticMeshComponent>();
	SetRootComponent(AppleStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	AppleStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Normal.json"));
}

void AAppleNormalActor::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
}

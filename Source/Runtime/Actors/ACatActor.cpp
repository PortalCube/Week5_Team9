#include "pch.h"
#include "ACatActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACatActor, AActor)
UCLASS_META(ACatActor, DisplayName, "Cat Actor")

ACatActor::ACatActor()
{	
	CatStaticMeshComp = NewObject<UStaticMeshComponent>();
	SetRootComponent(CatStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	CatStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/oiia.json"));
}

void ACatActor::Update(float DeltaTime)
{
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= SpinRate)
	{
		ElapsedTime = 0.0f;
		
		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		if (bIsSpin)
		{
			CatStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/oiia.json"));
			bIsSpin = false;
		}
		else
		{
			CatStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/Spin.json"));
			bIsSpin = true;
		}
	}

	if (bIsSpin)
	{
		FTransform CurrentTransform = GetTransform();		
		CurrentTransform.Rotation.RotateLocalAxisAngle(FVector(0.0f, 0.0f, 1.0f), SpinSpeed * DeltaTime);
		SetTransform(CurrentTransform);
	}
}

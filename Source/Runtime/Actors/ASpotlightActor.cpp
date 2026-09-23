#include "ASpotlightActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(ASpotlightActor, AActor)
UCLASS_META(ASpotlightActor, DisplayName, "Spotlight Actor")

ASpotlightActor::ASpotlightActor()
{
	CreateRootComponent(USpotLightComponent::StaticClass());

    FTransform DefaultTransform;
    DefaultTransform.Scale3D = FVector(5.0f, 5.0f, 5.0f);
    DefaultTransform.Rotation = FQuaternion::FromEulerXYZDeg(FVector(0.0, 90.0, 0.0f));
    SetTransform(DefaultTransform);
}

USpotLightComponent* ASpotlightActor::GetSpotlightComponent() const
{
	return RootComponent ? RootComponent->Cast<USpotLightComponent>() : nullptr;
}

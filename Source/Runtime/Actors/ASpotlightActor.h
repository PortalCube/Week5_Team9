#include "AActor.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"

class ASpotlightActor : public AActor
{
	DECLARE_UCLASS(ASpotlightActor, AActor)
	GENERATED_BODY()

public:
	explicit ASpotlightActor();

	USpotLightComponent* GetSpotlightComponent() const;
};

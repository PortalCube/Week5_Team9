#pragma once

#include "AActor.h"

class UStaticMeshComponent;

class AAppleNormalActor : public AActor
{
	DECLARE_UCLASS(AAppleNormalActor, AActor)
	GENERATED_BODY()

public:
	explicit AAppleNormalActor();

	virtual void Update(float DeltaTime) override;

private:
	UStaticMeshComponent* AppleStaticMeshComp;
};

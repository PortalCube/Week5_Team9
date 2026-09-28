#pragma once

#include "AActor.h"

class UStaticMeshComponent;

class AAppleBittenActor : public AActor
{
	DECLARE_UCLASS(AAppleBittenActor, AActor)
	GENERATED_BODY()

public:
	explicit AAppleBittenActor();

	virtual void Update(float DeltaTime) override;

private:
	UStaticMeshComponent* AppleStaticMeshComp;
};

#pragma once

#include "AActor.h"

// 큐브 액터 정의
class UStaticMeshComponent;

class ACatActor : public AActor
{
	DECLARE_UCLASS(ACatActor, AActor)
	GENERATED_BODY()

public:
	explicit ACatActor();

	virtual void Update(float DeltaTime) override;

private:
	UStaticMeshComponent* CatStaticMeshComp;
	bool bIsSpin = false;

	float SpinSpeed = 2000.0f;
	float SpinRate = 2.0f;
	float ElapsedTime = 0.0f;
};

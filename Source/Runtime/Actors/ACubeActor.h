#pragma once

#include "AActor.h"

// 큐브 액터 정의
class ACubeActor : public AActor
{
	DECLARE_UCLASS(ACubeActor, AActor)
	GENERATED_BODY()

public:
	explicit ACubeActor();
};

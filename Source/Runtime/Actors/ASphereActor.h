#pragma once

#include "AActor.h"

// 구체 액터 정의
class ASphereActor : public AActor
{
	DECLARE_UCLASS(ASphereActor, AActor)
	GENERATED_BODY()

public:
	explicit ASphereActor();
};

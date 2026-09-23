#pragma once

#include "AActor.h"
#include "Runtime/Math/FVector.h"

class UBillBoardComp;

// 큐브 액터 정의
class ABillboardActor : public AActor
{
	DECLARE_UCLASS(ABillboardActor, AActor)
	GENERATED_BODY()

public:
	explicit ABillboardActor();

	UBillBoardComp* GetBillboardComponent() const;
};

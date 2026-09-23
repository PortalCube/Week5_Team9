#pragma once

#include "AActor.h"
#include "Runtime/Math/FVector.h"

class UAnimatedBillboardComp;

// 애니메이션 빌보드 액터 정의
class AAnimatedBillboardActor : public AActor
{
	DECLARE_UCLASS(AAnimatedBillboardActor, AActor)
	GENERATED_BODY()

public:
	explicit AAnimatedBillboardActor();

	UAnimatedBillboardComp* GetAnimatedBillboardComponent() const;
};

using AnimatedBillboardActor = AAnimatedBillboardActor;

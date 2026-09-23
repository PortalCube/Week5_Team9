#pragma once

#include "Runtime/Actors/AActor.h"

class UTextInstanceComponent;

// 텍스트 인스턴스 액터 선언
class ATestTextActor : public AActor
{
	DECLARE_UCLASS(ATestTextActor, AActor)
	GENERATED_BODY()

public:
	explicit ATestTextActor();

	UTextInstanceComponent* GetTextInstanceComponent() const;
};

using TestTextActor = ATestTextActor;

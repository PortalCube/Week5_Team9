#pragma once

#include "AActor.h"
#include "Runtime/Math/FVector.h"

class UTextInstanceComponent;

// 큐브 액터 정의
class ATextRenderActor : public AActor
{
	DECLARE_UCLASS(ATextRenderActor, AActor)
	GENERATED_BODY()

public:
	explicit ATextRenderActor();

	UTextInstanceComponent* GetTextComponent() const;
};

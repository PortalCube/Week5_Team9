#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Engine/UScene.h"


// TODO: 언젠가는 USceneComponent로 옮길것..
class USpotLightComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(USpotLightComponent, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	explicit USpotLightComponent() = default;

public:
	void Initialize() override;

	float GetSpotAngle() const { return SpotAngle; }
	void SetSpotAngle(float InAngle) { SpotAngle = InAngle; }

	float GetRange() const { return Range; }
	void SetRange(float InRange) { Range = InRange; }

	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	const FVector& GetLightColor() const { return LightColor; }
	void SetLightColor(const FVector& InColor) { LightColor = InColor; }

	void Serialize(FArchive& Archive) const override;
	void Deserialize(const FArchive& Archive) override;

private:
	// 조명 기본 속성
	float SpotAngle = 30.0f;
	float Range = 5.0f;
	float Intensity = 1.0f;
	FVector LightColor{ 1.0f, 1.0f, 1.0f };
};

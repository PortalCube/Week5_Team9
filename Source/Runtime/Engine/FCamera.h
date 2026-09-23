#pragma once

#include "FCameraProjection.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FMatrix.h"

struct FCamera
{


	FVector Position{ 0.0f, 0.0f, 0.0f };
	float Yaw = 0.0f;
	float Pitch = 0.0f;
	FCameraProjection Projection;
	FVector UpVector{ 0.0f,0.0f,1.0f };

	// TODO: 캐시 가능, 캐시하려면 세터를 넣어야 함
	[[nodiscard]] FMatrix CreateViewProjectionMatrix() const;
	[[nodiscard]] FMatrix GetRotationMatrix() const;
	[[nodiscard]] FMatrix GetViewMatrix() const;
	[[nodiscard]] FMatrix GetProjectionMatrix() const;


	// Move(), Rotate(), Zoom() 등 추가 가능
};

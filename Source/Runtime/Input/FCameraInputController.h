#pragma once
#include "Runtime/Math/FVector.h"

struct FCamera;

class FCameraInputController
{
public:
	void UpdateKeyInput(FCamera& Camera, float DeltaTime);
	void UpdateMouseInput(FCamera& Camera) const;
	void UpdateMouseInput_ORTHOGRAPHIC(FCamera& Camera) const;

	float CameraMoveSpeed = 10.0f;
	float CameraRotateSpeed = 0.5f;

	// 가속·감속
	float Acceleration = 40.0f;   // 최대 속도까지 걸리는 정도. 클수록 빨리 붙음
	float Damping = 8.0f;    // 감속 강도. 클수록 빨리 멈춤

private:
	FVector Velocity;
};

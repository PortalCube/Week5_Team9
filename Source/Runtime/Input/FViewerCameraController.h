#pragma once
#include "Source/Runtime/Math/FVector.h"

struct FCamera;

class FViewerCameraController
{
public:
	void UpdateMouseInput(FCamera& Camera);

private:
	float CameraRotateSpeed = 0.5f;
	float CameraMovementSpeed = 0.5f;

	FVector TargetPosition = { 0.0f, 0.0f, 0.0f };
	float TargetDistance = 5.0f;
	float MinDistace = 0.5f;
	float MaxDistance = 100.0f;

	// Cemera Rotation
	float Yaw = 0.0f;
	float Pitch = 20.0f;
};
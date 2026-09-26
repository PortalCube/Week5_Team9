#include "FViewerCameraController.h"

#include "Source/Runtime/Input/FInputManager.h"
#include "Source/Runtime/Engine/FCamera.h"

#include "Source/Runtime/Math/FMatrix.h"

#include <algorithm>

void FViewerCameraController::UpdateMouseInput(FCamera& Camera)
{
	if (FInputManager::Get().IsMouseDown(EMouseButton::Left))
	{
		FVector2 Delta = FInputManager::Get().GetMouseDelta() * CameraRotateSpeed;
		Yaw += Delta.X;
		Pitch -= Delta.Y;
		Pitch = std::clamp(Pitch, -89.0f, 89.0f);
	}

	float Wheel = FInputManager::Get().GetMouseWheelDelta();
	if (abs(Wheel) >= 0.01f)
	{
		TargetDistance -= Wheel * CameraMovementSpeed;

		// clamp
		TargetDistance = std::clamp(TargetDistance, MinDistace, MaxDistance);
	}
	
	FMatrix Rotation = FMatrix::MakeRotation(FVector(0.0f, Pitch, Yaw));

	FVector Forward{ Rotation.M[0][0], Rotation.M[0][1], Rotation.M[0][2] };

	Camera.Position = TargetPosition - (Forward * TargetDistance);

	Camera.Pitch = Pitch;
	Camera.Yaw = Yaw;
}

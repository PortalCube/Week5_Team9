#include "FCameraInputController.h"

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FMatrix.h"
#include <Windows.h>

#include "FInputManager.h"
#include <algorithm>

void FCameraInputController::UpdateKeyInput(FCamera& Camera, float DeltaTime) 
{
	const FMatrix Rotation =
		FMatrix::MakeRotation(FVector(0.0f, Camera.Pitch, Camera.Yaw));

	const FVector Forward{
		Rotation.M[0][0], Rotation.M[0][1], Rotation.M[0][2]
	};
	const FVector Right{
		Rotation.M[1][0], Rotation.M[1][1], Rotation.M[1][2]
	};
	// 카메라 상방 벡터
	const FVector Up{
		Rotation.M[2][0], Rotation.M[2][1], Rotation.M[2][2]
	};

	FVector Direction{};

	float RelativeSpeed = 1.0f;

	if (FInputManager::Get().IsKeyDown(VK_LEFT) || FInputManager::Get().IsKeyDown('A'))
	{
		Direction -= Right;
	}

	if (FInputManager::Get().IsKeyDown(VK_RIGHT) || FInputManager::Get().IsKeyDown('D'))
	{
		Direction += Right;
	}

	if (FInputManager::Get().IsKeyDown(VK_UP) || FInputManager::Get().IsKeyDown('W'))
	{
		Direction += Forward;
	}

	if (FInputManager::Get().IsKeyDown(VK_DOWN) || FInputManager::Get().IsKeyDown('S'))
	{
		Direction -= Forward;
	}

	// 하강 이동
	if (FInputManager::Get().IsKeyDown('Q'))
	{
		Direction += FVector{0.0f, 0.0f, -1.0f};
	}

	// 상승 이동
	if (FInputManager::Get().IsKeyDown('E'))
	{
		Direction += FVector{ 0.0f, 0.0f, 1.0f };
	}

	if (FInputManager::Get().IsKeyDown(VK_LSHIFT))
	{
		RelativeSpeed *= 2.0f;
	}

	if (FInputManager::Get().IsKeyDown(VK_LCONTROL))
	{
		RelativeSpeed *= 0.5f;
	}

	// 대각선 정규화
	if (Direction.SizeSquared() > 0.0f)
	{
		Direction = (Direction /Direction.Size());
	}

	const FVector TargetVelocity = Direction * CameraMoveSpeed * RelativeSpeed;

	const float Rate = (Direction.SizeSquared() > 0.0f) ? Acceleration : Damping;
	const float Alpha = 1.0f - std::exp(-Rate * DeltaTime);

	Velocity += (TargetVelocity - Velocity) * Alpha;

	// 아주 느려지면 0으로 떨어뜨려 미세하게 떠다니는 것을 막는다
	if (Velocity.SizeSquared() < 0.0001f) { Velocity = FVector{}; }

	Camera.Position += Velocity * DeltaTime;
}

void FCameraInputController::UpdateMouseInput(FCamera& Camera) const
{
	if (FInputManager::Get().IsMouseDown(EMouseButton::Right))
	{
		FVector2 Delta = FInputManager::Get().GetMouseDelta() * CameraRotateSpeed;
		Camera.Yaw += Delta.X;
		Camera.Pitch -= Delta.Y;
		Camera.Pitch = std::clamp(Camera.Pitch, -89.0f, 89.0f);
	}
}

void FCameraInputController::UpdateMouseInput_ORTHOGRAPHIC(FCamera& Camera) const
{
	if (FInputManager::Get().IsMouseDown(EMouseButton::Right))
	{
		// 마우스의 클릭에 대한 원점은 좌측 상단
		const FVector2 Delta = FInputManager::Get().GetMouseDelta();
		
		const FMatrix Rotation = Camera.GetRotationMatrix();
		//카메라 로컬 +X방향
		const FVector Right{ Rotation.M[1][0], Rotation.M[1][1], Rotation.M[1][2] }; 
		//카메라 로컬 +Y방향
		const FVector Up{ Rotation.M[2][0], Rotation.M[2][1], Rotation.M[2][2] };
		
		const float PanSpeed = Camera.Projection.Height * 0.001f;
		Camera.Position += (Up * Delta.Y - Right * Delta.X) * PanSpeed;
	}
}
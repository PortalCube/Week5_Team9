#pragma once

#include "FCameraProjection.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FMatrix.h"

class FCamera
{
	FVector Position{ 0.0f, 0.0f, 0.0f };
	float Yaw = 0.0f;
	float Pitch = 0.0f;
	FCameraProjection Projection;
	FVector UpVector{ 0.0f, 0.0f, 1.0f };

	FMatrix RotationMatrix;
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	FMatrix ViewProjectionMatrix;

	void UpdateRotationMatrix();
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	void UpdateViewProjectionMatrix();

public:
	FCamera();

	const FVector& GetPosition() const { return Position; }
	float GetYaw() const { return Yaw; }
	float GetPitch() const { return Pitch; }
	const FCameraProjection& GetProjection() const { return Projection; }
	const FVector& GetUpVector() const { return UpVector; }

	void SetPosition(const FVector& Value);
	void SetYaw(float Value);
	void SetPitch(float Value);
	void SetRotation(float NewPitch, float NewYaw);
	void SetProjection(const FCameraProjection& Value);
	void SetProjectionType(EProjectionType Value);
	void SetFOV(float Value);
	void SetAspectRatio(float Value);
	void SetOrthographicHeight(float Value);
	void SetNearPlane(float Value);
	void SetFarPlane(float Value);
	void SetUpVector(const FVector& Value);

	const FMatrix& GetRotationMatrix() const { return RotationMatrix; }
	const FMatrix& GetViewMatrix() const { return ViewMatrix; }
	const FMatrix& GetProjectionMatrix() const { return ProjectionMatrix; }
	const FMatrix& GetViewProjectionMatrix() const { return ViewProjectionMatrix; }
	const FMatrix& CreateViewProjectionMatrix() const { return ViewProjectionMatrix; }
};

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
	FVector ForwardVector{ 1.0f, 0.0f, 0.0f };
	FVector RightVector{ 0.0f, 1.0f, 0.0f };

	mutable bool bRotationMatrixDirty = true;
	mutable bool bViewMatrixDirty = true;
	mutable bool bProjectionMatrixDirty = true;
	mutable bool bViewProjectionMatrixDirty = true;

	mutable FMatrix RotationMatrix;
	mutable FMatrix ViewMatrix;
	mutable FMatrix ProjectionMatrix;
	mutable FMatrix ViewProjectionMatrix;

	void UpdateRotationMatrixIfDirty() const;
	void UpdateDirectionVectors();
	void UpdateViewMatrixIfDirty() const;
	void UpdateProjectionMatrixIfDirty() const;
	void UpdateViewProjectionMatrixIfDirty() const;

public:
	FCamera();

	const FVector& GetPosition() const { return Position; }
	float GetYaw() const { return Yaw; }
	float GetPitch() const { return Pitch; }
	const FCameraProjection& GetProjection() const { return Projection; }
	const FVector& GetUpVector() const { return UpVector; }
	const FVector& GetForwardVector() const { return ForwardVector; }
	const FVector& GetRightVector() const { return RightVector; }

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

	const FMatrix& GetRotationMatrix() const;
	const FMatrix& GetViewMatrix() const;
	const FMatrix& GetProjectionMatrix() const;
	const FMatrix& GetViewProjectionMatrix() const;
};

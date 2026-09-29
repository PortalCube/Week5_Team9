#include "FCamera.h"

FCamera::FCamera()
{
}

void FCamera::SetPosition(const FVector& Value)
{
	Position = Value;
	bViewMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetYaw(float Value)
{
	Yaw = Value;
	bRotationMatrixDirty = true;
	bViewMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetPitch(float Value)
{
	Pitch = Value;
	bRotationMatrixDirty = true;
	bViewMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetRotation(float NewPitch, float NewYaw)
{
	Pitch = NewPitch;
	Yaw = NewYaw;
	bRotationMatrixDirty = true;
	bViewMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetProjection(const FCameraProjection& Value)
{
	Projection = Value;
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetProjectionType(EProjectionType Value)
{
	Projection.SetProjectionType(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetFOV(float Value)
{
	Projection.SetFOV(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetAspectRatio(float Value)
{
	if (Projection.GetAspectRatio() == Value)
	{
		return;
	}

	Projection.SetAspectRatio(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetOrthographicHeight(float Value)
{
	Projection.SetOrthographicHeight(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetNearPlane(float Value)
{
	Projection.SetNearPlane(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetFarPlane(float Value)
{
	Projection.SetFarPlane(Value);
	bProjectionMatrixDirty = true;
	bViewProjectionMatrixDirty = true;
}

void FCamera::SetUpVector(const FVector& Value)
{
	UpVector = Value;
}

const FMatrix& FCamera::GetRotationMatrix() const
{
	UpdateRotationMatrixIfDirty();
	return RotationMatrix;
}

const FMatrix& FCamera::GetViewMatrix() const
{
	UpdateViewMatrixIfDirty();
	return ViewMatrix;
}

const FMatrix& FCamera::GetProjectionMatrix() const
{
	UpdateProjectionMatrixIfDirty();
	return ProjectionMatrix;
}

const FMatrix& FCamera::GetViewProjectionMatrix() const
{
	UpdateViewProjectionMatrixIfDirty();
	return ViewProjectionMatrix;
}

void FCamera::UpdateRotationMatrixIfDirty() const
{
	if (!bRotationMatrixDirty)
	{
		return;
	}

	RotationMatrix = FMatrix::MakeRotation(FVector(0.0f, Pitch, Yaw));
	bRotationMatrixDirty = false;
}

void FCamera::UpdateViewMatrixIfDirty() const
{
	if (!bViewMatrixDirty)
	{
		return;
	}

	UpdateRotationMatrixIfDirty();
	ViewMatrix = FMatrix::MakeTranslation(-Position) * RotationMatrix.Transpose();
	bViewMatrixDirty = false;
}

void FCamera::UpdateProjectionMatrixIfDirty() const
{
	if (!bProjectionMatrixDirty)
	{
		return;
	}

	ProjectionMatrix = Projection.GetProjectionMatrix();
	bProjectionMatrixDirty = false;
}

void FCamera::UpdateViewProjectionMatrixIfDirty() const
{
	if (!bViewProjectionMatrixDirty)
	{
		return;
	}

	UpdateViewMatrixIfDirty();
	UpdateProjectionMatrixIfDirty();
	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;
	bViewProjectionMatrixDirty = false;
}

FVector FCamera::GetForwardVector() const
{
	const FMatrix& Rot = GetRotationMatrix();

	// 엔진의 FMatrix 멤버 변수 형태(M[0][0] 또는 m[0][0] 등)에 맞춰 작성합니다.
	FVector Forward(Rot.M[0][0], Rot.M[0][1], Rot.M[0][2]);
	Forward.Normalize();
	return Forward;
}

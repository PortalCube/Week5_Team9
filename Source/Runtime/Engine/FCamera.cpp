#include "FCamera.h"

FCamera::FCamera()
{
	UpdateRotationMatrix();
	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

void FCamera::SetPosition(const FVector& Value)
{
	Position = Value;
	UpdateViewMatrix();
}

void FCamera::SetYaw(float Value)
{
	Yaw = Value;
	UpdateRotationMatrix();
}

void FCamera::SetPitch(float Value)
{
	Pitch = Value;
	UpdateRotationMatrix();
}

void FCamera::SetRotation(float NewPitch, float NewYaw)
{
	Pitch = NewPitch;
	Yaw = NewYaw;
	UpdateRotationMatrix();
}

void FCamera::SetProjection(const FCameraProjection& Value)
{
	Projection = Value;
	UpdateProjectionMatrix();
}

void FCamera::SetProjectionType(EProjectionType Value)
{
	Projection.SetProjectionType(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetFOV(float Value)
{
	Projection.SetFOV(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetAspectRatio(float Value)
{
	Projection.SetAspectRatio(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetOrthographicHeight(float Value)
{
	Projection.SetOrthographicHeight(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetNearPlane(float Value)
{
	Projection.SetNearPlane(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetFarPlane(float Value)
{
	Projection.SetFarPlane(Value);
	UpdateProjectionMatrix();
}

void FCamera::SetUpVector(const FVector& Value)
{
	UpVector = Value;
}

void FCamera::UpdateRotationMatrix()
{
	RotationMatrix = FMatrix::MakeRotation(FVector(0.0f, Pitch, Yaw));
	UpdateViewMatrix();
}

void FCamera::UpdateViewMatrix()
{
	ViewMatrix = FMatrix::MakeTranslation(-Position) * RotationMatrix.Transpose();
	UpdateViewProjectionMatrix();
}

void FCamera::UpdateProjectionMatrix()
{
	ProjectionMatrix = Projection.GetProjectionMatrix();
	UpdateViewProjectionMatrix();
}

void FCamera::UpdateViewProjectionMatrix()
{
	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;
}

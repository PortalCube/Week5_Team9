#include "pch.h"
#include "FCameraProjection.h"

FCameraProjection::FCameraProjection()
{
	UpdateProjectionMatrix();
}

void FCameraProjection::UpdateProjectionMatrix()
{
	ProjectionMatrix = FMatrix{};
	
	if (ProjectionType == EProjectionType::Perspective)
	{
		const float Phi = FOV * std::numbers::pi_v<float> / 180.0f;
		const float C = 1.0f / std::tan(Phi * 0.5f);
		ProjectionMatrix.M[0][0] = FarZ / (FarZ - NearZ);
		ProjectionMatrix.M[1][1] = C / Aspect;
		ProjectionMatrix.M[2][2] = C;
		ProjectionMatrix.M[0][3] = 1.0f;
		ProjectionMatrix.M[3][0] = -NearZ * FarZ / (FarZ - NearZ);
	}
	else if (ProjectionType == EProjectionType::Orthographic)
	{
		const float OrthoWidth = Height * Aspect;
		ProjectionMatrix.M[0][0] = 1.0f / (FarZ - NearZ);
		ProjectionMatrix.M[1][1] = 2.0f / OrthoWidth;
		ProjectionMatrix.M[2][2] = 2.0f / Height;
		ProjectionMatrix.M[3][0] = -NearZ / (FarZ - NearZ);
		ProjectionMatrix.M[3][3] = 1.0f;
	}
}

void FCameraProjection::SetProjectionType(EProjectionType Value)
{
	ProjectionType = Value;
	UpdateProjectionMatrix();
}

void FCameraProjection::SetFOV(float Value)
{
	FOV = Value;
	UpdateProjectionMatrix();
}

void FCameraProjection::SetAspectRatio(float Value)
{
	Aspect = Value;
	UpdateProjectionMatrix();
}

void FCameraProjection::SetOrthographicHeight(float Value)
{
	Height = Value;
	UpdateProjectionMatrix();
}

void FCameraProjection::SetNearPlane(float Value)
{
	NearZ = Value;
	UpdateProjectionMatrix();
}

void FCameraProjection::SetFarPlane(float Value)
{
	FarZ = Value;
	UpdateProjectionMatrix();
}

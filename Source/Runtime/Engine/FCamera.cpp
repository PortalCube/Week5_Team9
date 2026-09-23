#include "FCamera.h"

FMatrix FCamera::GetRotationMatrix() const
{
	// 카메라 회전 행렬 반환
	return FMatrix::MakeRotation(FVector(0.0f, Pitch, Yaw));
}

FMatrix FCamera::GetViewMatrix() const
{
	return FMatrix::MakeTranslation(-Position) * GetRotationMatrix().Transpose();
}

FMatrix FCamera::GetProjectionMatrix() const
{
	// 카메라 투영 행렬 반환
	return Projection.CreateProjectionMatrix();
}

FMatrix FCamera::CreateViewProjectionMatrix() const
{
	// 뷰 행렬 계산
	const FMatrix ViewMatrix = GetViewMatrix();
	const FMatrix ProjectionMatrix = GetProjectionMatrix();

	return ViewMatrix * ProjectionMatrix;
}

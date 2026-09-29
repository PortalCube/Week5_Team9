#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FMatrix.h"

enum class EProjectionType : uint8
{
	Perspective,
	Orthographic,
};

class FCameraProjection
{

private:
	EProjectionType ProjectionType = EProjectionType::Perspective;
	float FOV = 60.0f; // Perspective 전용, Vertical
	float Aspect = 1.0f; // Perspective 전용. Width / Height
	float Height = 8.0f; // Orthographic 전용
	float NearZ = 0.1f;
	float FarZ = 100.0f;
	
	FMatrix ProjectionMatrix;

	// Perspective 전용. 반지름 R, 거리 D인 구의 화면 점유율은 ScreenSizeMultiple * R / D.
	// tan을 매번 계산하지 않도록 투영이 바뀔 때만 갱신한다.
	float ScreenSizeMultiple = 1.0f;
	void UpdateProjectionMatrix();

public:
	FCameraProjection();

	EProjectionType GetProjectionType() const { return ProjectionType; }
	float GetFOV() const { return FOV; }
	float GetAspectRatio() const { return Aspect; }
	float GetOrthographicHeight() const { return Height; }
	float GetNearPlane() const { return NearZ; }
	float GetFarPlane() const { return FarZ; }
	const FMatrix& GetProjectionMatrix() const { return ProjectionMatrix; }
	float GetScreenSizeMultiple() const { return ScreenSizeMultiple; }

	void SetProjectionType(EProjectionType Value);
	void SetFOV(float Value);
	void SetAspectRatio(float Value);
	void SetOrthographicHeight(float Value);
	void SetNearPlane(float Value);
	void SetFarPlane(float Value);
};

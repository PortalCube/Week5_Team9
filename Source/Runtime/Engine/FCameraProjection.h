#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FMatrix.h"

enum class EProjectionType : uint8
{
	Perspective,
	Orthographic,
};

struct FCameraProjection
{
	EProjectionType ProjectionType = EProjectionType::Perspective;
	float FOV = 60.0f; // Perspective 전용, Vertical
	float Aspect = 1.0f; // Perspective 전용. Width / Height
	float Height = 8.0f; // Orthographic 전용
	float NearZ = 0.1f;
	float FarZ = 100.0f;
	
	// TODO: 캐시 가능
	[[nodiscard]] FMatrix CreateProjectionMatrix() const;
};

inline FMatrix FCameraProjection::CreateProjectionMatrix() const
{
	FMatrix Matrix{ 0.0f };
	switch (ProjectionType)
	{
	case EProjectionType::Perspective:
	{
		const float Phi = FOV * std::numbers::pi_v<float> / 180.0f;
		const float C = 1.0f / std::tan(Phi * 0.5f);
		Matrix.M[0][0] = FarZ / (FarZ - NearZ);
		Matrix.M[1][1] = C / Aspect;
		Matrix.M[2][2] = C;
		Matrix.M[0][3] = 1.0f;
		Matrix.M[3][0] = -NearZ * FarZ / (FarZ - NearZ);
		break;
	}

	case EProjectionType::Orthographic:
		const float OrthoWidth = Height * Aspect;
		Matrix.M[0][0] = 1.0f / (FarZ - NearZ);
		Matrix.M[1][1] = 2.0f / OrthoWidth;
		Matrix.M[2][2] = 2.0f / Height;
		Matrix.M[3][0] = -NearZ / (FarZ - NearZ);
		Matrix.M[3][3] = 1.0f;
		break;
	}

	return Matrix;
}

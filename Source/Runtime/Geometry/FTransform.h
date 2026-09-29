#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Math/FMatrix.h"

class FTransform
{
	FVector Location{ 0.0f, 0.0f, 0.0f };
	FQuaternion Rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	FVector Scale3D{ 1.0f, 1.0f, 1.0f };

	mutable bool bTransformMatrixDirty = true;
	mutable FMatrix TransformMatrix;

	void UpdateTransformMatrixIfDirty() const;

public:
	const FVector& GetLocation() const { return Location; }
	const FQuaternion& GetRotation() const { return Rotation; }
	const FVector& GetScale3D() const { return Scale3D; }

	void SetLocation(const FVector& Value);
	void SetRotation(const FQuaternion& Value);
	void SetScale3D(const FVector& Value);

	const FMatrix& GetMatrix() const;

	// 부모 트랜스폼과 자식 트랜스폼 합성 연산자
	FTransform operator*(const FTransform& Child) const;

	bool operator==(const FTransform& Other) const
	{
		return Location == Other.Location
			&& Scale3D == Other.Scale3D
			&& Rotation.X == Other.Rotation.X && Rotation.Y == Other.Rotation.Y
			&& Rotation.Z == Other.Rotation.Z && Rotation.W == Other.Rotation.W;
	}
};

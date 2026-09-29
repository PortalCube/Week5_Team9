#include "FTransform.h"

void FTransform::UpdateTransformMatrixIfDirty() const
{
	if (!bTransformMatrixDirty)
	{
		return;
	}

	TransformMatrix = FMatrix::MakeScale(Scale3D) * Rotation.ToMatrixRow() * FMatrix::MakeTranslation(Location);
	bTransformMatrixDirty = false;
}

void FTransform::SetLocation(const FVector& Value)
{
	Location = Value;
	bTransformMatrixDirty = true;
}

void FTransform::SetRotation(const FQuaternion& Value)
{
	Rotation = Value;
	bTransformMatrixDirty = true;
}

void FTransform::SetScale3D(const FVector& Value)
{
	Scale3D = Value;
	bTransformMatrixDirty = true;
}

const FMatrix& FTransform::GetMatrix() const
{
	UpdateTransformMatrixIfDirty();
	return TransformMatrix;
}

FTransform FTransform::operator*(const FTransform& Child) const
{
	FTransform Result;

	Result.SetScale3D(
		FVector
		(
			Scale3D.X * Child.Scale3D.X,
			Scale3D.Y * Child.Scale3D.Y,
			Scale3D.Z * Child.Scale3D.Z
		)
	);

	Result.SetRotation((Rotation * Child.Rotation).Normalized());

	const FVector ScaledChildLocation
	(
		Child.Location.X * Scale3D.X,
		Child.Location.Y * Scale3D.Y,
		Child.Location.Z * Scale3D.Z
	);

	Result.SetLocation(Location + Rotation.RotateVector(ScaledChildLocation));

	return Result;
}

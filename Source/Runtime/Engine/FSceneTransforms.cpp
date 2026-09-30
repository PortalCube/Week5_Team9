#include "FSceneTransforms.h"
#include "Runtime/Engine/UScene.h"
#include <malloc.h>

void FSceneTransforms::Initialize(size_t InCapacity)
{
	AllocatedCapacity = (InCapacity + 3) & ~3; // 4개 단위 SIMD 정렬
	const size_t FloatBufferSize = sizeof(float) * AllocatedCapacity;

	PosX = (float*)_aligned_malloc(FloatBufferSize, 16);
	PosY = (float*)_aligned_malloc(FloatBufferSize, 16);
	PosZ = (float*)_aligned_malloc(FloatBufferSize, 16);
	RotX = (float*)_aligned_malloc(FloatBufferSize, 16);
	RotY = (float*)_aligned_malloc(FloatBufferSize, 16);
	RotZ = (float*)_aligned_malloc(FloatBufferSize, 16);
	RotW = (float*)_aligned_malloc(FloatBufferSize, 16);
	ScaleX = (float*)_aligned_malloc(FloatBufferSize, 16);
	ScaleY = (float*)_aligned_malloc(FloatBufferSize, 16);
	ScaleZ = (float*)_aligned_malloc(FloatBufferSize, 16);

	WorldMatrices = (FMatrix*)_aligned_malloc(sizeof(FMatrix) * AllocatedCapacity, 16);
}

void FSceneTransforms::ShutDown()
{
	_aligned_free(PosX);
	_aligned_free(PosY);
	_aligned_free(PosZ);
	_aligned_free(RotX);
	_aligned_free(RotY);
	_aligned_free(RotZ);
	_aligned_free(RotW);
	_aligned_free(ScaleX);
	_aligned_free(ScaleY);
	_aligned_free(ScaleZ);
	_aligned_free(WorldMatrices);

	PosX = PosY = PosZ = nullptr;
	RotX = RotY = RotZ = RotW = nullptr;
	ScaleX = ScaleY = ScaleZ = nullptr;
	WorldMatrices = nullptr;

	AllocatedCapacity = 0;
}

void FSceneTransforms::Reserve(int32 NewCapacity)
{
	if (NewCapacity <= AllocatedCapacity) return;

	size_t TargetCapacity = AllocatedCapacity == 0 ? 64 : AllocatedCapacity + (AllocatedCapacity >> 1);
	if (TargetCapacity < (size_t)NewCapacity)
	{
		TargetCapacity = (size_t)NewCapacity;
	}
	TargetCapacity = (TargetCapacity + 3) & ~3;

	const size_t FloatBytes = sizeof(float) * TargetCapacity;
	const size_t MatrixBytes = sizeof(FMatrix) * TargetCapacity;

	float* NewPosX = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewPosY = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewPosZ = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewRotX = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewRotY = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewRotZ = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewRotW = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewScaleX = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewScaleY = (float*)_aligned_malloc(FloatBytes, 16);
	float* NewScaleZ = (float*)_aligned_malloc(FloatBytes, 16);
	FMatrix* NewWorldMatrices = (FMatrix*)_aligned_malloc(MatrixBytes, 16);

	if(AllocatedCapacity > 0)
	{
		const size_t OldFloatBytes = sizeof(float) * AllocatedCapacity;
		memcpy_s(NewPosX, FloatBytes, PosX, OldFloatBytes);
		memcpy_s(NewPosY, FloatBytes, PosY, OldFloatBytes);
		memcpy_s(NewPosZ, FloatBytes, PosZ, OldFloatBytes);
		memcpy_s(NewRotX, FloatBytes, RotX, OldFloatBytes);
		memcpy_s(NewRotY, FloatBytes, RotY, OldFloatBytes);
		memcpy_s(NewRotZ, FloatBytes, RotZ, OldFloatBytes);
		memcpy_s(NewRotW, FloatBytes, RotW, OldFloatBytes);
		memcpy_s(NewScaleX, FloatBytes, ScaleX, OldFloatBytes);
		memcpy_s(NewScaleY, FloatBytes, ScaleY, OldFloatBytes);
		memcpy_s(NewScaleZ, FloatBytes, ScaleZ, OldFloatBytes);
		memcpy_s(NewWorldMatrices, MatrixBytes, WorldMatrices, sizeof(FMatrix) * AllocatedCapacity);

		ShutDown();
	}

	PosX = NewPosX; PosY = NewPosY; PosZ = NewPosZ;
	RotX = NewRotX; RotY = NewRotY; RotZ = NewRotZ; RotW = NewRotW;
	ScaleX = NewScaleX; ScaleY = NewScaleY; ScaleZ = NewScaleZ;
	WorldMatrices = NewWorldMatrices;
	AllocatedCapacity = TargetCapacity;
}

void FSceneTransforms::SetTransform(int32 Index, const FTransform& Transform)
{
	const FVector& Position = Transform.GetLocation();
	const FQuaternion& Rotation = Transform.GetRotation();
	const FVector& Scale = Transform.GetScale3D();

	PosX[Index] = Position.X;
	PosY[Index] = Position.Y;
	PosZ[Index] = Position.Z;
	RotX[Index] = Rotation.X;
	RotY[Index] = Rotation.Y;
	RotZ[Index] = Rotation.Z;
	RotW[Index] = Rotation.W;
	ScaleX[Index] = Scale.X;
	ScaleY[Index] = Scale.Y;
	ScaleZ[Index] = Scale.Z;
}

void FSceneTransforms::UpdateWorldMatrices(const UScene& Scene)
{
	const auto& DirtyIndices = Scene.GetDirtyTransformIndices();
	const auto& Components = Scene.GetRenderComponents();

	for (int32 Index : DirtyIndices)
	{
		if (Index >= 0 && Index < static_cast<int32>(Components.size()) && Components[Index])
		{
			const float qx = RotX[Index], qy = RotY[Index], qz = RotZ[Index], qw = RotW[Index];
			const float sx = ScaleX[Index], sy = ScaleY[Index], sz = ScaleZ[Index];
			const float px = PosX[Index], py = PosY[Index], pz = PosZ[Index];

			const float x2 = qx + qx, y2 = qy + qy, z2 = qz + qz;
			const float xx = qx * x2, yy = qy * y2, zz = qz * z2;
			const float xy = qx * y2, xz = qx * z2, yz = qy * z2;
			const float wx = qw * x2, wy = qw * y2, wz = qw * z2;

			float* M = reinterpret_cast<float*>(&WorldMatrices[Index]);

			M[0] = sx * (1.0f - (yy + zz));
			M[1] = sx * (xy + wz);
			M[2] = sx * (xz - wy);
			M[3] = 0.0f;

			M[4] = sy * (xy - wz);
			M[5] = sy * (1.0f - (xx + zz));
			M[6] = sy * (yz + wx);
			M[7] = 0.0f;

			M[8] = sz * (xz + wy);
			M[9] = sz * (yz - wx);
			M[10] = sz * (1.0f - (xx + yy));
			M[11] = 0.0f;

			M[12] = px;
			M[13] = py;
			M[14] = pz;
			M[15] = 1.0f;
		}
	}

	const_cast<UScene&>(Scene).ClearDirtyTransformIndices();
}

void FSceneTransforms::ComputeBatchMVP(const FMatrix& InViewProj, FMatrix* OutMVPMatrices, int32 Count) const
{
	float* VP = reinterpret_cast<float*>(const_cast<FMatrix*>(&InViewProj));
	const auto VPRow0 = FMathSSE::VectorLoadAligned(VP);
	const auto VPRow1 = FMathSSE::VectorLoadAligned(VP + 4);
	const auto VPRow2 = FMathSSE::VectorLoadAligned(VP + 8);
	const auto VPRow3 = FMathSSE::VectorLoadAligned(VP + 12);

	for (int32 i = 0; i < Count; ++i)
	{
		const float* SrcWorld = reinterpret_cast<float*>(&WorldMatrices[i]);
		float* DstMVP = reinterpret_cast<float*>(&OutMVPMatrices[i]);

		for (int32 Row = 0; Row < 4; ++Row)
		{
			auto WorldRow = FMathSSE::VectorLoadAligned(SrcWorld + Row * 4);

			auto X = FMathSSE::VectorReplicate<0>(WorldRow);
			auto Y = FMathSSE::VectorReplicate<1>(WorldRow);
			auto Z = FMathSSE::VectorReplicate<2>(WorldRow);
			auto W = FMathSSE::VectorReplicate<3>(WorldRow);

			auto Res = FMathSSE::VectorMul(X, VPRow0);
			Res = FMathSSE::VectorAdd(Res, FMathSSE::VectorMul(Y, VPRow1));
			Res = FMathSSE::VectorAdd(Res, FMathSSE::VectorMul(Z, VPRow2));
			Res = FMathSSE::VectorAdd(Res, FMathSSE::VectorMul(W, VPRow3));

			FMathSSE::VectorStoreAligned(Res, DstMVP + Row * 4);
		}
	}
}
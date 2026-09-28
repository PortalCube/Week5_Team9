#include "FSceneTransforms.h"
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
	const FVector& Position = Transform.Location;
	const FQuaternion& Rotation = Transform.Rotation;
	const FVector& Scale = Transform.Scale3D;

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

void FSceneTransforms::UpdateWorldMatrices(int32 Count)
{
	for (int32 i = 0; i < Count; i += 4)
	{
		FMathSSE::VectorRegister4Float ScaleXVec = FMathSSE::VectorLoadAligned(&ScaleX[i]);
		FMathSSE::VectorRegister4Float ScaleYVec = FMathSSE::VectorLoadAligned(&ScaleY[i]);
		FMathSSE::VectorRegister4Float ScaleZVec = FMathSSE::VectorLoadAligned(&ScaleZ[i]);

		FMathSSE::VectorRegister4Float RotXVec = FMathSSE::VectorLoadAligned(&RotX[i]);
		FMathSSE::VectorRegister4Float RotYVec = FMathSSE::VectorLoadAligned(&RotY[i]);
		FMathSSE::VectorRegister4Float RotZVec = FMathSSE::VectorLoadAligned(&RotZ[i]);
		FMathSSE::VectorRegister4Float RotWVec = FMathSSE::VectorLoadAligned(&RotW[i]);

		FMathSSE::VectorRegister4Float PosXVec = FMathSSE::VectorLoadAligned(&PosX[i]);
		FMathSSE::VectorRegister4Float PosYVec = FMathSSE::VectorLoadAligned(&PosY[i]);
		FMathSSE::VectorRegister4Float PosZVec = FMathSSE::VectorLoadAligned(&PosZ[i]);

		FMathSSE::VectorRegister4Float X2 = FMathSSE::VectorAdd(RotXVec, RotXVec);
		FMathSSE::VectorRegister4Float Y2 = FMathSSE::VectorAdd(RotYVec, RotYVec);
		FMathSSE::VectorRegister4Float Z2 = FMathSSE::VectorAdd(RotZVec, RotZVec);

		FMathSSE::VectorRegister4Float XX = FMathSSE::VectorMul(RotXVec, X2);
		FMathSSE::VectorRegister4Float YY = FMathSSE::VectorMul(RotYVec, Y2);
		FMathSSE::VectorRegister4Float ZZ = FMathSSE::VectorMul(RotZVec, Z2);

		FMathSSE::VectorRegister4Float XY = FMathSSE::VectorMul(RotXVec, Y2);
		FMathSSE::VectorRegister4Float XZ = FMathSSE::VectorMul(RotXVec, Z2);
		FMathSSE::VectorRegister4Float YZ = FMathSSE::VectorMul(RotYVec, Z2);

		FMathSSE::VectorRegister4Float WX = FMathSSE::VectorMul(RotWVec, X2);
		FMathSSE::VectorRegister4Float WY = FMathSSE::VectorMul(RotWVec, Y2);
		FMathSSE::VectorRegister4Float WZ = FMathSSE::VectorMul(RotWVec, Z2);

		const auto One = FMathSSE::VectorSetFloat1(1.0f);
		const auto Zero = FMathSSE::VectorSetFloat1(0.0f);

		FMathSSE::VectorRegister4Float M00 = FMathSSE::VectorMul(ScaleXVec, FMathSSE::VectorSub(One, FMathSSE::VectorAdd(YY, ZZ)));
		FMathSSE::VectorRegister4Float M01 = FMathSSE::VectorMul(ScaleXVec, FMathSSE::VectorAdd(XY, WZ));
		FMathSSE::VectorRegister4Float M02 = FMathSSE::VectorMul(ScaleXVec, FMathSSE::VectorSub(XZ, WY));
		FMathSSE::VectorRegister4Float M03 = Zero;

		FMathSSE::VectorRegister4Float M10 = FMathSSE::VectorMul(ScaleYVec, FMathSSE::VectorSub(XY, WZ));
		FMathSSE::VectorRegister4Float M11 = FMathSSE::VectorMul(ScaleYVec, FMathSSE::VectorSub(One, FMathSSE::VectorAdd(XX, ZZ)));
		FMathSSE::VectorRegister4Float M12 = FMathSSE::VectorMul(ScaleYVec, FMathSSE::VectorAdd(YZ, WX));
		FMathSSE::VectorRegister4Float M13 = Zero;

		FMathSSE::VectorRegister4Float M20 = FMathSSE::VectorMul(ScaleZVec, FMathSSE::VectorAdd(XZ, WY));
		FMathSSE::VectorRegister4Float M21 = FMathSSE::VectorMul(ScaleZVec, FMathSSE::VectorSub(YZ, WX));
		FMathSSE::VectorRegister4Float M22 = FMathSSE::VectorMul(ScaleZVec, FMathSSE::VectorSub(One, FMathSSE::VectorAdd(XX, YY)));
		FMathSSE::VectorRegister4Float M23 = Zero;

		FMathSSE::VectorRegister4Float M30 = PosXVec;
		FMathSSE::VectorRegister4Float M31 = PosYVec;
		FMathSSE::VectorRegister4Float M32 = PosZVec;
		FMathSSE::VectorRegister4Float M33 = One;

		_MM_TRANSPOSE4_PS(M00, M01, M02, M03);
		_MM_TRANSPOSE4_PS(M10, M11, M12, M13);
		_MM_TRANSPOSE4_PS(M20, M21, M22, M23);
		_MM_TRANSPOSE4_PS(M30, M31, M32, M33);

		float* Dst0 = reinterpret_cast<float*>(&WorldMatrices[i]);
		FMathSSE::VectorStoreAligned(M00, Dst0);
		FMathSSE::VectorStoreAligned(M10, Dst0 + 4);
		FMathSSE::VectorStoreAligned(M20, Dst0 + 8);
		FMathSSE::VectorStoreAligned(M30, Dst0 + 12);

		float* Dst1 = reinterpret_cast<float*>(&WorldMatrices[i + 1]);
		FMathSSE::VectorStoreAligned(M01, Dst1);
		FMathSSE::VectorStoreAligned(M11, Dst1 + 4);
		FMathSSE::VectorStoreAligned(M21, Dst1 + 8);
		FMathSSE::VectorStoreAligned(M31, Dst1 + 12);

		float* Dst2 = reinterpret_cast<float*>(&WorldMatrices[i + 2]);
		FMathSSE::VectorStoreAligned(M02, Dst2);
		FMathSSE::VectorStoreAligned(M12, Dst2 + 4);
		FMathSSE::VectorStoreAligned(M22, Dst2 + 8);
		FMathSSE::VectorStoreAligned(M32, Dst2 + 12);

		float* Dst3 = reinterpret_cast<float*>(&WorldMatrices[i + 3]);
		FMathSSE::VectorStoreAligned(M03, Dst3);
		FMathSSE::VectorStoreAligned(M13, Dst3 + 4);
		FMathSSE::VectorStoreAligned(M23, Dst3 + 8);
		FMathSSE::VectorStoreAligned(M33, Dst3 + 12);
	}
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
#include "FTransformSoAManager.h"

void FTransformSoAManager::Initialize(size_t InCapacity)
{
	AllocatedCapacity = (InCapacity + 3) & ~3; // Align to 4 for SIMD
	const size_t FloatBufferSize = sizeof(float) * AllocatedCapacity;

	PosX = (float*)FMemory::Malloc(FloatBufferSize, 16);
	PosY = (float*)FMemory::Malloc(FloatBufferSize, 16);
	PosZ = (float*)FMemory::Malloc(FloatBufferSize, 16);
	RotX = (float*)FMemory::Malloc(FloatBufferSize, 16);
	RotY = (float*)FMemory::Malloc(FloatBufferSize, 16);
	RotZ = (float*)FMemory::Malloc(FloatBufferSize, 16);
	RotW = (float*)FMemory::Malloc(FloatBufferSize, 16);
	ScaleX = (float*)FMemory::Malloc(FloatBufferSize, 16);
	ScaleY = (float*)FMemory::Malloc(FloatBufferSize, 16);
	ScaleZ = (float*)FMemory::Malloc(FloatBufferSize, 16);

	WorldMatrices = (FMatrix*)FMemory::Malloc(sizeof(FMatrix) * AllocatedCapacity, 16);
}

void FTransformSoAManager::ShutDown()
{
	FMemory::Free(PosX);
	FMemory::Free(PosY);
	FMemory::Free(PosZ);
	FMemory::Free(RotX);
	FMemory::Free(RotY);
	FMemory::Free(RotZ);
	FMemory::Free(RotW);
	FMemory::Free(ScaleX);
	FMemory::Free(ScaleY);
	FMemory::Free(ScaleZ);
	FMemory::Free(WorldMatrices);

	PosX = nullptr;
	PosY = nullptr;
	PosZ = nullptr;
	RotX = nullptr;
	RotY = nullptr;
	RotZ = nullptr;
	RotW = nullptr;
	ScaleX = nullptr;
	ScaleY = nullptr;
	ScaleZ = nullptr;
	WorldMatrices = nullptr;

	AllocatedCapacity = 0;
}

void FTransformSoAManager::UpdateWorldMatrices(int32 Count)
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
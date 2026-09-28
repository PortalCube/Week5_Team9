#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/MathSSE.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Math/FMatrix.h"

class FTransformSoAManager
{
public:
	float* PosX = nullptr, * PosY = nullptr, * PosZ = nullptr;
	float* RotX = nullptr, * RotY = nullptr, * RotZ = nullptr, * RotW = nullptr;
	float* ScaleX = nullptr, * ScaleY = nullptr, * ScaleZ = nullptr;

	FMatrix* WorldMatrices = nullptr;

	size_t AllocatedCapacity = 0;

	void Initialize(size_t InCapacity);
	void UpdateWorldMatrices(int32 Count);
	void ShutDown();
};
#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/MathSSE.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Geometry/FTransform.h"

class UScene;

class FSceneTransforms
{
public:
	float* PosX = nullptr, * PosY = nullptr, * PosZ = nullptr;
	float* RotX = nullptr, * RotY = nullptr, * RotZ = nullptr, * RotW = nullptr;
	float* ScaleX = nullptr, * ScaleY = nullptr, * ScaleZ = nullptr;

	FMatrix* WorldMatrices = nullptr;

	size_t AllocatedCapacity = 0;

	void Initialize(size_t InCapacity);
	void ShutDown();
	void Reserve(int32 NewCapacity);
	void SetTransform(int32 Index, const FTransform& Transform);
	void UpdateWorldMatrices(const UScene& Scene);
	void ComputeBatchMVP(const FMatrix& InViewProj, FMatrix* OutMVPMatrices, int32 Count) const;
};
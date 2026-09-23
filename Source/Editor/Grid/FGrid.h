#pragma once
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"

struct FCamera;

class FGrid
{
private:
	float CellSize = 1.0f;
	bool bIsActive = true;
public:
	void DrawLine(FRenderer& Renderer, const FCamera& Camera);

	float GetCellSize() const { return CellSize; }
	void SetCellSize(float InCellSize) { CellSize = (InCellSize > 0.01f) ? InCellSize : 0.01f; }
};
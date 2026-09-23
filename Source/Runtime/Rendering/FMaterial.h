#pragma once

#include "FRenderPipeline.h"
#include "FTexture.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Material/FTextureSamplerDesc.h"
#include "Vertices.h"
#include <d3d11.h>

class FMaterial final {
	friend class FRenderer;

public:

	void SetPipeLine(FRenderPipeline* InPipeline) { Pipeline = InPipeline; }
	FRenderPipeline* GetPipeline() const { return Pipeline; }

	void SetTexture(FTexture* InTexture) { Texture = InTexture; }
	FTexture* GetTexture() const { return Texture; }

	void SetSamplerDesc(FTextureSamplerDesc InSamplerDesc) { SamplerDesc = InSamplerDesc; }
	FTextureSamplerDesc GetSamplerDesc() const { return SamplerDesc; }

	FName MaterialId{ "None" };

	void BindResources(ID3D11DeviceContext& Context) const;

private:

	FRenderPipeline* Pipeline = nullptr;
	FTexture* Texture = nullptr;
	FTextureSamplerDesc SamplerDesc;
};

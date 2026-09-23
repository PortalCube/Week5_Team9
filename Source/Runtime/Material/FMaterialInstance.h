#pragma once

#include "Runtime/Material/FTextureSamplerDesc.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Asset/UPipeline.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UTexture.h"

#include "Runtime/Utility/EngineUtil.h"

class UMaterial;
class UTexture;
class UPipeline;

struct FMaterialInstance
{
	// 원본이 되는 Material.
	UMaterial* Material = nullptr;

	UPipeline* Pipeline = nullptr;
	UTexture* Texture = nullptr;
	FTextureSamplerDesc SamplerDesc{};

	// ==== Shader Constant Buffer ====
	
	FVector2 UVOffset = { 0.0f, 0.0f };
	FVector2 UVScale = { 1.0f, 1.0f };

	bool bDisableShading = false;

	float Albedo = 1.0f;
	float Diffuse = 1.0f;
	float Specular = 1.0f;

	FVector4 Color = { 1.0f, 1.0f, 1.0f, 0.0f };

	// ================================

	FMaterialInstance(UMaterial* Parent)
		: Material{ Parent }
	{
		if (Material == nullptr)
		{
			throw EngineUtil::CreateError("[FMaterialInstance] 부모 Material이 nullptr입니다.");
		}

		Pipeline = Material->GetPipeline();
		Texture = Material->GetTexture();
		SamplerDesc = Material->GetSamplerDesc();
	}

	// TODO: 위에 [Shader Constant Buffer] 영역을 치우고
	// 재질에 대한 Parameter (Albedo, Diffuse, Specular)를 Dynamic하게 받아오는 기능 추가.
	// 이게 쉐이더마다 받아올 Parameter가 다르므로...
	// 이걸 구현하려면 언리얼식 Reflection 시스템이 필요할 것
};

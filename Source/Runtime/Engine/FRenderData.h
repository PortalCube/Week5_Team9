#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Material/FMaterialInstance.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/ShaderConstants.h"

enum class ERenderType
{
    Primitive,
    Texture,
    Text,
    Instancing,
    Spotlight,
    None
};

// 런타임 게임 로직에서 생성하는 렌더 정보
struct FRenderData
{
    UStaticMesh* Mesh = nullptr;
    TArray<FMaterialInstance> Materials;
    FMatrix ModelMatrix = FMatrix::Identity;

    ERenderType Type = ERenderType::Primitive;
    TArray<FInstanceData> Instances;
};

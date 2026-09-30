#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Material/FMaterialInstance.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/ShaderConstants.h"

enum class ERenderType
{
    Primitive,
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

    // 이번에 그릴 LOD. GetRenderData(Camera) 호출 시 카메라 기준으로 갱신된다.
    uint32 LODIndex = 0;

    // 파이프라인/머티리얼/텍스처로 구성된 SortKey 상위 48비트.
    // 메시와 LOD로 구성되는 하위 16비트는 DrawCommand 생성 시 결합한다.
    uint64 SortKey = 0;
};

#pragma once

#include "Runtime/Engine/FRenderData.h"
#include "FMesh.h"
#include "FMaterial.h"
#include "ShaderConstants.h"
#include "Runtime/Core/TArray.h"

// 렌더링에 필요한 드로우 정보
struct FDrawCommand
{
    FMesh* Mesh = nullptr;
    //TArray<FMaterial> Materials;
	std::span<const FMaterial> Materials;
    FObjectConstants Constants{};
    ERenderType Type = ERenderType::Primitive;
    //TArray<FInstanceData> Instances;
	std::span<const FInstanceData> Instances;
    float Depth = 0.0f;
    int32 DepthBucket = 0;
    uint64 RenderStateKey = 0;
};

// 한 프레임의 드로우 요청을 수집하는 큐
class FRenderQueue
{
public:
    // 아이템 추가
    void Push(FDrawCommand&& Data)
    {
        switch (Data.Type)
        {
        case ERenderType::Primitive:
            primRenderQ.push_back(std::move(Data));
            break;
        case ERenderType::Text:
            TextRenderQ.push_back(std::move(Data));
            break;
        case ERenderType::Instancing:
            InstancingRenderQ.push_back(std::move(Data));
            break;
        case ERenderType::Spotlight:
            SpotlightRenderQ.push_back(std::move(Data));
            break;
        default:
            break;
        }
    }

    // 수집된 아이템 조회
    const TArray<FDrawCommand>& GetPrimRenderQ() const { return primRenderQ; }
    const TArray<FDrawCommand>& GetTextRenderQ() const { return TextRenderQ; }
    const TArray<FDrawCommand>& GetInstancingRenderQ() const { return InstancingRenderQ; }
    const TArray<FDrawCommand>& GetSpotlightRenderQ() const { return SpotlightRenderQ; }

    // 프레임 끝에 호출
    void Clear() { 
        primRenderQ.clear();
        TextRenderQ.clear();
        InstancingRenderQ.clear();
        SpotlightRenderQ.clear();
    }

    void Sort();

    bool IsPrimRQEmpty() const { return primRenderQ.empty(); }
    bool IsTextRQEmpty() const { return TextRenderQ.empty(); }
    bool IsInstancingRQEmpty() const { return InstancingRenderQ.empty(); }
    bool IsSpotlightRQEmpty() const { return SpotlightRenderQ.empty(); }

private:
    TArray<FDrawCommand> primRenderQ;
    TArray<FDrawCommand> TextRenderQ;
    TArray<FDrawCommand> InstancingRenderQ;
    TArray<FDrawCommand> SpotlightRenderQ;
};

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
    TArray<FMaterial> Materials;
    FObjectConstants Constants{};
    ERenderType Type = ERenderType::Primitive;
    TArray<FInstanceData> Instances;
};

// 한 프레임의 드로우 요청을 수집하는 큐
class FRenderQueue
{
public:
    // 아이템 추가
    void Push(const FDrawCommand& Data)
    {
        switch (Data.Type)
        {
        case ERenderType::Primitive:
            primRenderQ.push_back(Data);
            break;
        case ERenderType::Texture:
            TextureRenderQ.push_back(Data);
            break;
        case ERenderType::Text:
            TextRenderQ.push_back(Data);
            break;
        case ERenderType::Instancing:
            InstancingRenderQ.push_back(Data);
            break;
        case ERenderType::Spotlight:
            SpotlightRenderQ.push_back(Data);
            break;
        default:
            break;
        }
    }

    // 수집된 아이템 조회
    const TArray<FDrawCommand>& GetPrimRenderQ() const { return primRenderQ; }
    const TArray<FDrawCommand>& GetTextureRenderQ() const { return TextureRenderQ; }
    const TArray<FDrawCommand>& GetTextRenderQ() const { return TextRenderQ; }
    const TArray<FDrawCommand>& GetInstancingRenderQ() const { return InstancingRenderQ; }
    const TArray<FDrawCommand>& GetSpotlightRenderQ() const { return SpotlightRenderQ; }

    // 프레임 끝에 호출
    void Clear() { 
        primRenderQ.clear();
        TextureRenderQ.clear();
        TextRenderQ.clear();
        InstancingRenderQ.clear();
        SpotlightRenderQ.clear();
    }

    bool IsPrimRQEmpty() const { return primRenderQ.empty(); }
    bool IsTextureRQEmpty() const { return TextureRenderQ.empty(); }
    bool IsTextRQEmpty() const { return TextRenderQ.empty(); }
    bool IsInstancingRQEmpty() const { return InstancingRenderQ.empty(); }
    bool IsSpotlightRQEmpty() const { return SpotlightRenderQ.empty(); }

private:
    TArray<FDrawCommand> primRenderQ;
    TArray<FDrawCommand> TextureRenderQ;
    TArray<FDrawCommand> TextRenderQ;
    TArray<FDrawCommand> InstancingRenderQ;
    TArray<FDrawCommand> SpotlightRenderQ;
};

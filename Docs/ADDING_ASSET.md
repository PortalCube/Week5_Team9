# 애셋 추가하기

마지막 업데이트: 5주차 (2026-09-26)

[온보딩 문서로 돌아가기](./ONBOARDING.md)

이 문서에서는 Oiiaii 엔진에서 애셋을 추가하는 방법에 대해서 다룹니다.

에셋의 유형과 설명에 대해서는 여기서 다루지 않습니다. 아래 문서를 확인해주세요.

- [애셋 살펴보기](./ASSET_OVERVIEW.md)

## 메타데이터

모든 애셋은 JSON 형태로 메타데이터를 가지고 있습니다.

엔진은 `Content` 폴더에 위치하는 모든 JSON 메타데이터를 애셋으로 인식합니다. 애셋을 추가할 때는 JSON을 추가하시면 됩니다.

또한 `Content` 폴더를 보시면 `Font`, `Material` 등 유형별로 정리되어 있는 모습을 볼 수 있습니다.

이는 단순히 작업자가 보기 편한 분류대로 모아두었을 뿐, 반드시 Material 폴더에 UMaterial을 위치 해야하는 것은 아닙니다.

(예: `Content/MyGame/TitleImage.json`으로 새로운 `UTexture`를 생성해도 정상적으로 인식합니다.)

## 알아두면 좋을 것

- "상대 경로"라고 표시된 부분은 `Content` 폴더를 루트로 하는 상대 경로를 의미합니다.
    - `Content/MyGame/TitleImage.json`은 `MyGame/TitleImage.json`으로 나타냅니다.

- 애셋 ID는 메타데이터 JSON의 상대 경로입니다. 예를 들어 `Content/Texture/White.json`의 ID는 `Texture/White.json`입니다.
- 경로 구분자는 `/`를 사용하는 것을 권장합니다. 파일명과 참조 경로의 대소문자도 정확히 맞춰주세요.
- 런타임은 실행 파일 옆의 `Content` 폴더를 읽습니다. 저장소의 `Content`만 수정했다면 빌드 후 복사되었는지 확인해주세요.
- 로드 순서는 `Pipeline` → `Texture` → `Material` → `Font` → `StaticMesh`입니다. 따라서 메타데이터 파일의 디렉터리 순서와 무관하게 참조 대상이 먼저 로드됩니다.

## UAsset 공통 데이터

| Key       | Value          | Required? | 설명                                                                                               |
| --------- | -------------- | --------- | -------------------------------------------------------------------------------------------------- |
| Version   | int32 (1)      | Yes       | Metadata의 Schema를 가리킵니다.                                                                    |
| Name      | FString (utf8) | Yes       | 애셋의 이름을 가리킵니다. 개발자나 유저들에게 직접 노출되는 값입니다. 다른 애셋과 중복 가능합니다. |
| AssetType | 문자열         | Yes       | `Pipeline`, `Texture`, `Material`, `Font`, `StaticMesh` 중 하나입니다.                             |

## UTexture

| Key                | Value     | Required? | 설명                                         |
| ------------------ | --------- | --------- | -------------------------------------------- |
| AssetType          | "Texture" | Yes       | `UTexture` 유형의 애셋임을 가리킵니다.       |
| RawTextureFilePath | 상대 경로 | Yes       | png, jpg, dds 유형의 파일 경로를 가리킵니다. |

## UMaterial

| Key                       | Value               | Required? | 설명                                                                                    |
| ------------------------- | ------------------- | --------- | --------------------------------------------------------------------------------------- |
| AssetType                 | "Material"          | Yes       | `UMaterial` 유형의 애셋임을 가리킵니다.                                                 |
| UPipelineID               | 상대 경로           | Yes       | 참조할 UPipeline 애셋의 ID를 가리킵니다.                                                |
| UTextureID                | 상대 경로 또는 null | No        | 참조할 `UTexture` 애셋의 ID입니다. 텍스처가 필요 없으면 `null`로 지정하거나 비워둡니다. |
| TextureSampler.FilterMode | FilterMode          | Yes       | 텍스처의 필터링 모드를 나타냅니다.                                                      |
| TextureSampler.WrapMode   | WrapMode            | Yes       | 텍스처가 UV 좌표를 벗어났을 때의 주소 지정 모드입니다.                                  |

### FilterMode

| Type        | 설명                                                               |
| ----------- | ------------------------------------------------------------------ |
| Point       | 가장 가까운 텍셀 하나를 사용합니다. 선명하지만 계단 현상이 큽니다. |
| Bilinear    | 인접한 4개 텍셀을 보간합니다.                                      |
| Trilinear   | 두 밉 레벨의 bilinear 결과를 추가로 보간합니다.                    |
| Anisotropic | 비스듬한 표면에서 품질이 좋은 이방성 필터링을 사용합니다.          |

### WrapMode

| Type   | 설명                                        |
| ------ | ------------------------------------------- |
| Wrap   | UV를 반복합니다.                            |
| Mirror | 반복되는 구간마다 UV를 반전합니다.          |
| Clamp  | UV를 0~1 범위의 가장자리 값으로 고정합니다. |

## UFont

| Key        | Value     | Required? | 설명                                                            |
| ---------- | --------- | --------- | --------------------------------------------------------------- |
| AssetType  | "Font"    | Yes       | `UFont` 유형의 애셋임을 가리킵니다.                             |
| UTextureID | 상대 경로 | Yes       | 글리프 아틀라스로 사용할 `UTexture` 애셋의 ID입니다.            |
| GlyphData  | object    | Yes       | MSDF 폰트의 JSON 데이터입니다. `atlas`와 `glyphs`를 포함합니다. |

`GlyphData`는 `msdfgen.exe`가 출력한 JSON 객체를 그대로 넣으시면 됩니다.

## UStaticMesh

| Key          | Value        | Required? | 설명                                                                                                                                                                              |
| ------------ | ------------ | --------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| AssetType    | "StaticMesh" | Yes       | `UStaticMesh` 유형의 애셋임을 가리킵니다.                                                                                                                                         |
| MeshFilePath | 상대 경로    | Yes       | obj 유형의 파일 경로를 가리킵니다. <br/>주의: bin 파일을 가리키지 마세요. 지금은 obj 파일을 엔진이 직접 베이킹하도록 되어있고, obj 파일 옆에 bin 파일이 있으면 알아서 가져옵니다. |

## UPipeline

| Key                         | Value          | Required? | 설명                                            |
| --------------------------- | -------------- | --------- | ----------------------------------------------- |
| AssetType                   | "Pipeline"     | Yes       | `UPipeline` 유형의 애셋임을 가리킵니다.         |
| VertexShaderFilePath        | 상대 경로      | Yes       | 컴파일된 Vertex Shader(`.cso`)의 경로입니다.    |
| PixelShaderFilePath         | 상대 경로      | Yes       | 컴파일된 Pixel Shader(`.cso`)의 경로입니다.     |
| Instancing                  | bool           | Yes       | 인스턴싱용 입력 레이아웃을 사용할지 나타냅니다. |
| Rasterizer.FillMode         | FillMode       | Yes       | 폴리곤을 채우는 방식을 지정합니다.              |
| Rasterizer.CullMode         | CullMode       | Yes       | 제거할 면을 지정합니다.                         |
| Rasterizer.FrontFaceMode    | FrontFaceMode  | Yes       | 앞면으로 판정할 정점 감기 방향을 지정합니다.    |
| Rasterizer.Multisample      | bool           | Yes       | 멀티샘플 래스터라이징 사용 여부입니다.          |
| Rasterizer.AntialiasedLine  | bool           | Yes       | 안티앨리어싱된 선 렌더링 사용 여부입니다.       |
| DepthStencil.DepthEnable    | bool           | Yes       | 깊이 테스트 사용 여부입니다.                    |
| DepthStencil.StencilEnable  | bool           | Yes       | 스텐실 테스트 사용 여부입니다.                  |
| DepthStencil.DepthWriteMode | DepthWriteMode | Yes       | 깊이 버퍼 쓰기 여부를 지정합니다.               |
| Blend.BlendMode             | BlendMode      | Yes       | 색상 혼합 방식을 지정합니다.                    |

### FillMode

| Type      | 설명                        |
| --------- | --------------------------- |
| Solid     | 삼각형 내부를 채웁니다.     |
| Wireframe | 삼각형의 외곽선만 그립니다. |

### CullMode

| Type  | 설명                         |
| ----- | ---------------------------- |
| None  | 어느 면도 제거하지 않습니다. |
| Front | 앞면을 제거합니다.           |
| Back  | 뒷면을 제거합니다.           |

### FrontFaceMode

| Type             | 설명                                           |
| ---------------- | ---------------------------------------------- |
| CounterClockwise | 반시계 방향으로 감긴 삼각형을 앞면으로 봅니다. |
| Clockwise        | 시계 방향으로 감긴 삼각형을 앞면으로 봅니다.   |

### DepthWriteMode

| Type    | 설명                       |
| ------- | -------------------------- |
| Disable | 깊이 버퍼에 쓰지 않습니다. |
| Enable  | 깊이 버퍼에 씁니다.        |

### BlendMode

| Type               | 설명                                         |
| ------------------ | -------------------------------------------- |
| Opaque             | 불투명 렌더링을 사용합니다.                  |
| Masked             | 알파 마스킹용 혼합 모드를 사용합니다.        |
| Translucent        | 일반적인 알파 블렌딩을 사용합니다.           |
| Additive           | 원본 색상을 대상 색상에 더합니다.            |
| PremultipliedAlpha | 미리 곱해진 알파를 사용하는 혼합 모드입니다. |

## 예시

### Material

```json
{
    "Version": 1,
    "Name": "Textured",
    "AssetType": "Material",
    "UPipelineID": "Pipeline/Textured.json",
    "UTextureID": "Texture/White.json",
    "TextureSampler": {
        "FilterMode": "Bilinear",
        "WrapMode": "Wrap"
    }
}
```

### Pipeline

```json
{
    "Version": 1,
    "Name": "Textured",
    "AssetType": "Pipeline",
    "VertexShaderFilePath": "Shader/ExampleVS.cso",
    "PixelShaderFilePath": "Shader/TexturedPS.cso",
    "Rasterizer": {
        "FillMode": "Solid",
        "CullMode": "Back",
        "FrontFaceMode": "Clockwise",
        "Multisample": false,
        "AntialiasedLine": false
    },
    "Blend": {
        "BlendMode": "Translucent"
    },
    "DepthStencil": {
        "DepthEnable": true,
        "StencilEnable": true,
        "DepthWriteMode": "Enable"
    },
    "Instancing": false
}
```

주의: bin 파일을 가리키지 마세요. 지금은 obj 파일을 게임이 직접 베이킹하여 bin 파일을 알아서 가리킵니다.

## 수정된 Content 폴더를 배포하기

소스코드에 있는 Content 폴더는 버전 관리를 위한 원본으로, 이 폴더에 있는 애셋은 바이너리에 로드되지 않습니다.

Content 폴더를 수정했으면 아래의 방법 중 하나로 수정된 Content 폴더를 바이너리 바로 옆에 위치시켜서 로드될 수 있도록 해야합니다.

### 코드를 다시 빌드한다

- 빌드 스크립트가 실행되어 Content 폴더가 바이너리 폴더 옆에 위치하게 됩니다.

### RunBuildScript_Debug.bat 혹은 RunBuildScript_Release.bat을 실행한다

- 코드 수정 없이 애셋만 수정했을 때 유용합니다. 빌드 스크립트를 수동으로 실행하여 바이너리 폴더 옆에 위치시킵니다.

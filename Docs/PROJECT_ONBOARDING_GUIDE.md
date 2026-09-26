# Oiiaii 엔진 프로젝트 온보딩 가이드

> 이 문서는 새 팀원이 저장소를 받은 뒤 **빌드하고, 에디터를 실행하고, 코드 구조를 이해하고, 첫 기능을 안전하게 추가하는 데 필요한 정보**를 한곳에 모은 예시 문서입니다.
>
> 기준 코드: 2026-09-25 현재 저장소. 프로젝트가 바뀌면 코드가 문서보다 우선합니다.

---

## 0. 가장 먼저 읽을 요약

Oiiaii는 Windows에서 실행되는 C++20 기반의 작은 DirectX 11 에디터/엔진 프로젝트입니다. Win32가 창과 입력 이벤트를 받고, DirectX 11이 장면을 렌더링하며, Dear ImGui가 에디터 UI를 구성합니다. 프로젝트 파일은 Premake5로 생성합니다.

처음 합류한 날에는 아래 순서대로 진행하면 됩니다.

1. Visual Studio 2026의 C++ 데스크톱 개발 도구와 Premake5를 준비합니다.
2. 저장소의 Git submodule을 초기화합니다.
3. 루트의 `GenerateProjects.bat`을 실행합니다.
4. 생성된 `MyEngine.sln`을 Visual Studio에서 엽니다.
5. `x64 / Debug`로 빌드하고 실행합니다.
6. `Binaries/x64/Debug` 아래 실행 파일과 `Content` 복사 여부를 확인합니다.
7. 코드를 보기 시작할 때는 `Source/main.cpp` → `FEditorApplication` → `FEditor` → `UScene` → `FRenderView` 순서를 권장합니다.

프로젝트를 이해하는 핵심 문장은 다음과 같습니다.

> `main.cpp`가 플랫폼과 메인 루프를 소유하고, `FEditorApplication`이 에디터 한 프레임을 조율하며, `UScene`이 액터를 소유하고, `FRenderView`가 씬을 화면에 그립니다.

---

## 1. 프로젝트의 목적과 현재 범위

이 프로젝트는 Unreal Engine과 유사한 일부 개념을 직접 구현하며 렌더링 엔진과 에디터의 기본 구조를 학습하는 팀 프로젝트입니다. 코드에서 확인되는 주요 기능은 다음과 같습니다.

- Win32 창 생성 및 메시지 처리
- DirectX 11 렌더러 초기화와 프레임 출력
- Dear ImGui 기반 에디터 UI
- 단일/분할/쿼드 에디터 뷰포트
- Perspective 및 Orthographic 카메라
- Actor/Component 기반 씬 구성
- 런타임 클래스 등록과 문자열 기반 객체 생성
- 씬 직렬화 및 역직렬화
- JSON 메타데이터 기반 에셋 로딩
- OBJ 정적 메시 가져오기
- 머티리얼, 파이프라인, 텍스처, 폰트, 정적 메시 에셋
- 기즈모, 선택, 아웃라이너, 프로퍼티, 콘텐츠 드로어, 콘솔, 통계 UI
- 인스턴싱, 빌보드, 텍스트, 스포트라이트와 에디터용 시각화

프로젝트는 범용 상용 엔진이 아니라 빠르게 변화하는 학습용 코드베이스입니다. 공개 API라고 생각하고 코드를 확장하기보다, 호출 경로와 객체 수명 주기를 먼저 확인하는 편이 안전합니다.

---

## 2. 기술 스택

| 영역 | 사용 기술 |
|---|---|
| 언어 | C++20, HLSL, PowerShell, Lua(Premake) |
| 플랫폼 | Windows, Win32 API |
| 그래픽스 | Direct3D 11, DXGI |
| UI | Dear ImGui Win32/DX11 백엔드 |
| 프로젝트 생성 | Premake5 |
| IDE/컴파일러 | Visual Studio 2026, MSVC v145 |
| 외부 라이브러리 | DirectXTK, nlohmann/json, stb_image, mINI |
| 데이터 | JSON 기반 에셋, `.Scene` 씬 파일, OBJ/MTL/BIN, DDS |

`premake5.lua`에 정의된 workspace 이름은 `MyEngine`, 시작 프로젝트도 `MyEngine`입니다. 실행 창 제목은 현재 `OIIAII`입니다.

---

## 3. 개발 환경 준비

### 3.1 필수 준비물

- Windows 개발 환경
- Visual Studio 2026
  - Desktop development with C++ 워크로드
  - Windows SDK
  - MSVC v145 toolset
- Git
- Premake5
- PowerShell

Premake5가 없다면 다음과 같이 설치할 수 있습니다.

```powershell
winget install --id Premake.Premake.5.Beta
```

설치 후 새 터미널을 열고 확인합니다.

```powershell
premake5 --version
```

### 3.2 저장소 받기

처음 clone할 때는 submodule까지 함께 받는 방법이 가장 간단합니다.

```powershell
git clone --recurse-submodules <repository-url>
cd Week5_Team9
```

이미 clone했다면 다음 명령으로 DirectXTK submodule을 초기화합니다.

```powershell
git submodule update --init --recursive
```

현재 `.gitmodules`에는 `Source/ThirdParty/DirectXTK`가 등록되어 있습니다.

### 3.3 Visual Studio 프로젝트 생성

프로젝트 루트에서 다음 중 하나를 실행합니다.

```powershell
.\GenerateProjects.bat
```

또는:

```powershell
premake5 vs2026
```

다음 경우에는 프로젝트를 다시 생성하는 습관을 들이는 것이 좋습니다.

- `.cpp`, `.h`, `.hlsl` 파일을 추가·이동·삭제했을 때
- 다른 팀원의 파일 구조 변경을 pull했을 때
- `premake5.lua`를 수정했을 때
- 솔루션/프로젝트 파일이 없거나 Visual Studio 필터가 이상할 때

Visual Studio가 외부 변경을 감지하면 `모두 다시 로드`를 선택합니다.

---

## 4. 빌드와 실행

### 4.1 기본 빌드

1. `MyEngine.sln`을 엽니다.
2. 플랫폼을 `x64`로 선택합니다.
3. 구성을 `Debug`, `Release`, `ObjViewer` 중 하나로 선택합니다.
4. `MyEngine`을 시작 프로젝트로 빌드/실행합니다.

| 구성 | 용도 | 출력 경로 |
|---|---|---|
| Debug | 일반 개발 및 디버깅 | `Binaries/x64/Debug` |
| Release | 최적화된 일반 실행 | `Binaries/x64/Release` |
| ObjViewer | `_OBJVIEWER` 전용 애플리케이션 | `Binaries/x64/ObjViewer` |

Win32 빌드도 정의되어 있지만, 새 개발은 특별한 이유가 없다면 x64를 기준으로 삼는 편이 좋습니다.

### 4.2 빌드 중 자동으로 일어나는 일

Premake 설정은 모든 정상 빌드 앞뒤에 다음 스크립트를 연결합니다.

```text
빌드 시작
  -> Scripts/PreBuild.ps1
     -> Scripts/PreBuild/*.ps1을 이름순 실행
     -> Content 아래 원본 이미지를 DDS로 변환
  -> C++ 및 HLSL 빌드
     -> VS/PS 셰이더를 Content/Shader/*.cso로 출력
  -> Scripts/PostBuild.ps1
     -> Scripts/PostBuild/*.ps1을 이름순 실행
     -> Content 전체를 실행 파일 옆으로 복사
빌드 완료
```

런타임은 저장소 루트의 `Content`가 아니라 **실행 파일과 같은 디렉터리에 복사된 `Content`**를 읽습니다. `EngineUtil::GetContentDirectory()`가 실행 파일의 부모 디렉터리를 기준으로 경로를 만들기 때문입니다.

### 4.3 코드 변경 없이 애셋만 바꾼 경우

Visual Studio가 컴파일을 생략하면 pre/post build hook도 실행되지 않을 수 있습니다. 이때는 다음 스크립트를 수동 실행합니다.

```powershell
.\RunBuildScript_Debug.bat
```

또는:

```powershell
.\RunBuildScript_Release.bat
```

두 스크립트 모두 첫 번째 인자로 다른 출력 디렉터리를 받을 수 있습니다.

```powershell
.\RunBuildScript_Debug.bat "D:\Temp\MyEngineOutput"
```

### 4.4 성공 확인 체크리스트

- 애플리케이션 창 제목이 `OIIAII`로 보입니다.
- 에디터 UI와 뷰포트가 표시됩니다.
- `Binaries/x64/<구성>/Content`가 존재합니다.
- `Content/Shader` 아래에 컴파일된 `.cso`가 있습니다.
- 텍스처가 흰색 또는 오류 텍스처로만 보이지 않습니다.
- Visual Studio Output 창에 치명적인 asset/schema 오류가 없습니다.

---

## 5. 저장소 구조

```text
Week5_Team9/
├─ Content/                  런타임이 읽는 에셋 정의와 원본/변환 리소스
│  ├─ Font/
│  ├─ Material/
│  ├─ Pipeline/
│  ├─ Shader/                빌드된 셰이더(.cso)
│  ├─ StaticMesh/
│  └─ Texture/
├─ Docs/                     팀 문서와 문서용 이미지/영상
├─ Fonts/                    레거시 또는 작업용 폰트 데이터
├─ OtherResources/           PSD 등 원본 작업 파일
├─ Resources/                원본/샘플 리소스
├─ SceneData/                저장된 Scene 파일 예시
├─ Scripts/
│  ├─ PreBuild/              빌드 전 변환 작업
│  ├─ PostBuild/             빌드 후 배포 작업
│  └─ ClangFormat.ps1        코드 포맷 스크립트
├─ Shader/                   HLSL 소스
├─ Source/
│  ├─ Editor/                에디터 애플리케이션, UI, 기즈모, 뷰포트
│  ├─ Runtime/               코어, 객체, 씬, 렌더링, 입력, 에셋
│  └─ ThirdParty/            외부 라이브러리
├─ premake5.lua              빌드/프로젝트 생성의 기준 파일
├─ GenerateProjects.bat      Visual Studio 프로젝트 생성
└─ RunBuildScript_*.bat      애셋 전처리/복사를 수동 실행
```

중요한 원칙은 다음과 같습니다.

- 직접 수정하는 셰이더 소스는 `Shader`에 있습니다.
- 런타임 에셋 메타데이터는 `Content`에 있습니다.
- 빌드 결과인 `Binaries`와 중간 결과인 `Intermediate`는 소스 코드와 구분합니다.
- 서드파티 코드는 프로젝트 코드와 동일한 스타일로 고치지 않습니다.

---

## 6. 프로그램 시작부터 종료까지

진입점은 `Source/main.cpp`의 `wWinMain`입니다. 초기화 순서를 바꾸는 작업은 파급 범위가 크므로 아래 흐름을 이해해야 합니다.

### 6.1 초기화 순서

1. Win32 창 생성
2. `FInputManager` singleton 생성
3. `FRenderer` 생성 및 DirectX 11 초기화
4. `FRenderView` 생성
5. `FStatsManager`와 `FMemory` 초기화
6. `FRenderResourceLibrary` 초기화
7. `UClass::ResolveTypeBitsets()`로 클래스 상속 정보 확정
8. `FResourceLoader::LoadAssets()`로 코드 생성 및 Content 에셋 로드
9. 일반 구성이라면 `USceneManager`와 빈 `UScene` 생성
10. `FEditorApplication`의 ImGui 및 런타임 초기화
11. ObjViewer 구성이라면 대신 `FObjViewerApplication` 초기화

클래스 등록이 끝나기 전에 타입 판별을 사용하거나, 렌더 리소스 라이브러리보다 에셋을 먼저 로드하지 않도록 주의합니다.

### 6.2 매 프레임의 흐름

```text
FTimeManager::Update
  -> Win32 메시지 처리
  -> resize 요청 반영
  -> FInputManager::BeginFrame
  -> FEditorApplication::Update
     -> ImGui NewFrame
     -> 각 에디터 패널 Process
     -> FEditor::Process
        -> Scene::Update
        -> 선택 Actor transform 반영
        -> editor state 저장/flush 예약
  -> FRenderer::BeginFrame
  -> FEditorApplication::Render
     -> FRenderView::PrepareRender
     -> 활성 viewport마다 RenderView
     -> overlay/gizmo 렌더링
     -> ImGui UI 렌더링
  -> FRenderer::SwapBuffer
```

새 시스템을 추가할 때는 기능 성격에 따라 위치를 선택합니다.

- 플랫폼 이벤트: `main.cpp`의 window callback
- 에디터 UI의 프레임 처리: `FEditorApplication::Tick`
- 씬의 게임/객체 업데이트: `UScene::Update` 또는 `AActor::Update`
- 화면 출력: `FRenderView`, `FRenderer`, 렌더 큐/파이프라인 계층

### 6.3 종료 순서

일반 에디터 구성에서는 에디터 상태를 저장한 뒤 씬을 해제하고 렌더러를 종료합니다. 객체의 `Release`, `Unregister`, `EndPlay`를 우회해 직접 메모리만 해제하면 레지스트리나 씬의 비소유 포인터가 남을 수 있습니다.

---

## 7. 핵심 아키텍처

### 7.1 계층별 책임

| 계층 | 대표 타입 | 책임 |
|---|---|---|
| 플랫폼 | `wWinMain`, `WindowCallback` | 창, 메시지, 원시 입력, resize |
| 애플리케이션 | `FEditorApplication` | 에디터 update/render 조율 |
| 에디터 코어 | `FEditor`, `FEditorState` | 선택, 기즈모, 뷰포트, 상태 |
| UI | `FImgui*Window`, `FImguiToolBar` | 화면 패널과 사용자 명령 |
| 씬 | `USceneManager`, `UScene` | 현재 씬, 액터 수명, 저장/로드 |
| 객체 | `UObject`, `UClass`, `AActor`, Components | 타입, 생성, 수명, 구성 |
| 에셋 | `FResourceLoader`, `FAssetRegistry`, `UAsset` | 파일 검색, 의존 순서 로드, 조회 |
| 렌더링 | `FRenderer`, `FRenderView`, `FRenderResourceLibrary` | D3D11 자원과 렌더 패스 |
| 입력 | `FInputManager`, `FCameraInputController` | 키/마우스 상태와 카메라 조작 |

### 7.2 객체 모델

`UObject`는 엔진 객체의 뿌리입니다. 각 객체는 UUID와 내부 인덱스를 가지며 복사와 이동이 금지되어 있습니다. `UObject` 파생 객체는 일반 `new`보다 `NewObject<T>()`를 통해 만들어야 객체 배열과 커스텀 할당/수명 체계에 참여합니다.

파생 클래스에는 보통 다음 패턴이 필요합니다.

```cpp
class AExampleActor : public AActor
{
	DECLARE_UCLASS(AExampleActor, AActor)
	GENERATED_BODY()

protected:
	AExampleActor() = default;

public:
	void Initialize() override;
};
```

```cpp
IMPLEMENT_UCLASS(AExampleActor, AActor)
UCLASS_META(AExampleActor, DisplayName, "Example Actor")
```

역할은 다음과 같습니다.

- `GENERATED_BODY()`는 `NewObject`가 생성자에 접근할 수 있게 합니다.
- `DECLARE_UCLASS`는 타입 정보와 factory 등록을 선언합니다.
- `IMPLEMENT_UCLASS`는 생성 함수와 타입 조회를 구현합니다.
- `UCLASS_META`는 에디터 표시명이나 직렬화명 같은 메타데이터를 붙입니다.

새 타입이 문자열 기반 씬 로드나 에디터 생성 목록에 나타나야 한다면 등록 매크로 누락 여부를 가장 먼저 확인합니다.

### 7.3 Actor와 Component

- `AActor`는 씬에 들어가는 논리 객체입니다.
- `USceneComponent`는 transform과 부착 관계, 등록/플레이 수명 주기를 가집니다.
- `UPrimitiveComponent`는 mesh/material/texture 등의 `FRenderData`를 제공하며 렌더 가능한 component의 기반입니다.
- `AActor::RootComponent`의 transform이 일반적인 actor transform 역할을 합니다.

일반 수명 흐름은 다음과 같습니다.

```text
NewObject
  -> Initialize
  -> Scene에 추가
  -> Register
  -> BeginPlay
  -> Update 반복
  -> EndPlay
  -> Unregister
  -> Release / DestroyObject
```

`UScene`은 액터 배열과 렌더 컴포넌트 배열을 별도로 보유합니다. `UPrimitiveComponent::Register`/`Unregister`는 이 렌더 목록과 연결되므로 등록 과정을 건너뛰면 객체가 존재해도 화면에 나오지 않을 수 있습니다.

### 7.4 Scene과 SceneManager

`USceneManager`는 현재 씬을 소유하고 전환, 저장, 로드를 담당합니다. `SetScene`은 이전 씬을 종료/비활성화/파괴하고 새 씬을 초기화/활성화/시작합니다.

현재 저장 포맷의 schema version은 2입니다.

```json
{
  "Version": 2,
  "NextUUID": 42,
  "Scene": {
    "Type": "Scene",
    "Actors": []
  }
}
```

실제 필드는 각 타입의 `Serialize` 구현에 따라 더 추가됩니다. 부모 필드를 보존하려면 override에서 `Super::Serialize(Archive)`와 `Super::Deserialize(Archive)` 호출을 유지합니다.

주의: 저장소의 일부 오래된 `SceneData/*.Scene` 파일은 version 1 형식일 수 있습니다. 현재 `USceneManager::LoadScene`은 version 2만 허용하므로, 오래된 샘플이 로드되지 않는 현상을 빌드 오류로 오해하지 마십시오.

### 7.5 렌더링

`FRenderer`는 장치, context, swap chain 등 저수준 DirectX 11 작업을 담당합니다. `FRenderView`는 씬과 카메라/뷰 설정을 받아 실제 렌더 패스를 구성합니다.

각 활성 viewport마다 `FSceneView`가 만들어지며 다음 정보를 포함합니다.

- 카메라와 view-projection matrix
- 화면 내 viewport 위치와 크기
- view mode와 show flags
- global light constants

에디터 전용 데이터는 `FEditorRenderContext`로 별도 전달됩니다.

- 선택된 actor/component
- 선택 transform
- gizmo와 선택 actor용 text component
- grid
- visualizer registry

따라서 런타임 장면 표현과 에디터 오버레이를 섞지 않고 확장하는 것이 현재 구조와 잘 맞습니다.

### 7.6 에셋 로딩

`FResourceLoader::LoadAssets()`는 실행 파일 옆의 `Content` 디렉터리를 재귀 순회하고 `.json` 파일을 읽습니다. 파일 경로의 `Content` 기준 상대 경로가 Asset ID가 됩니다.

현재 로드 순서는 의존성을 고려해 고정되어 있습니다.

```text
Pipeline -> Texture -> Material -> Font -> StaticMesh
```

모든 에셋 JSON은 현재 `Version: 1`과 올바른 `AssetType`이 필요합니다. 지원 타입은 다음과 같습니다.

- `Pipeline`
- `Texture`
- `Material`
- `Font`
- `StaticMesh`

이름이 중복된 에셋을 `FAssetRegistry`에 등록하면 예외가 발생합니다. 코드 생성 내부 리소스는 대체로 `#` 접두사를 사용하며 콘텐츠 브라우저에서도 일반 에셋과 분리됩니다.

---

## 8. 에디터 사용법

### 8.1 주요 패널

`FEditorApplication::Tick` 기준으로 다음 UI가 매 프레임 처리됩니다.

- Tool Bar: 파일과 뷰 설정
- Editor Viewport: 씬 표시와 카메라/선택 상호작용
- World Outliner: 씬 actor 목록
- Control Panel: actor 생성 등 제어
- Property Window: 선택 객체 속성 편집
- Console Window: 명령 입력과 로그
- Contents Drawer: 등록된 에셋 탐색
- Stats Window: FPS/메모리 통계

### 8.2 파일 메뉴

- `New Scene`: 빈 씬 생성
- `Save Scene`: 현재 경로에 저장, 경로가 없으면 저장 대화상자 표시
- `Save scene as...`: 새 경로에 저장
- `Load Scene`: `.Scene` 파일 로드
- `Import Import`: 현재 구현상 OBJ 파일 가져오기 메뉴입니다. 이름은 오타성 UI 텍스트로 보입니다.

파일 대화상자는 `OFN_NOCHANGEDIR`를 사용합니다. 프로세스의 current working directory가 바뀌어 상대 경로 에셋 로딩이 깨지는 것을 방지하기 위한 의도입니다.

### 8.3 뷰포트와 카메라

View 메뉴에서 다음 배치를 선택할 수 있습니다.

- Single
- Top | Bottom
- Left | Right
- 2 × 2

Perspective 카메라의 기본 이동은 다음과 같습니다.

| 입력 | 동작 |
|---|---|
| `W/S` 또는 위/아래 방향키 | 전진/후진 |
| `A/D` 또는 좌/우 방향키 | 좌/우 이동 |
| `Q/E` | 하강/상승 |
| 마우스 오른쪽 드래그 | 시점 회전 |
| 왼쪽 Shift | 이동 속도 2배 |
| 왼쪽 Ctrl | 이동 속도 절반 |

Orthographic 뷰에서 마우스 오른쪽 드래그는 회전 대신 pan으로 동작합니다.

선택된 actor는 `Delete` 키로 삭제됩니다. 기즈모 모드는 상단 버튼을 누를 때 `None → Translation → Rotation → Scale` 순서로 순환합니다.

`main.cpp`에는 F5/F6/F7 요청 플래그가 존재하지만 현재 검색 기준으로 실제 씬 저장/로드/생성 처리와 연결되어 있지 않습니다. 동작이 확인되기 전까지 단축키로 문서화하거나 의존하지 않는 편이 안전합니다.

### 8.4 콘솔 명령

현재 확인되는 명령은 다음과 같습니다.

```text
stat memory
stat fps
stat none
```

대소문자는 내부에서 소문자로 바꾸어 비교합니다.

---

## 9. 새 기능을 추가하는 대표 절차

### 9.1 새 Actor 추가

예를 들어 `AExampleActor`를 만든다면 다음을 확인합니다.

1. `Source/Runtime/Actors`에 헤더와 cpp를 추가합니다.
2. `AActor`를 상속합니다.
3. `DECLARE_UCLASS`, `GENERATED_BODY`, `IMPLEMENT_UCLASS`를 추가합니다.
4. 에디터 표시가 필요하면 `DisplayName` metadata를 추가합니다.
5. `Initialize`에서 root component를 만들고 필요한 mesh/material을 설정합니다.
6. 상태 저장이 필요하면 `Serialize`/`Deserialize`를 구현하고 `Super`를 호출합니다.
7. `premake5 vs2026` 또는 `GenerateProjects.bat`을 다시 실행합니다.
8. 에디터에서 생성, 선택, 저장, 로드, 삭제를 모두 시험합니다.

간단한 기존 예제는 `ACubeActor`, `ASphereActor`, `ACylinderActor`를 참고하고, component가 더 복잡한 예는 `AAnimatedBillboardActor`, `ATextRenderActor`, `ASpotlightActor`를 참고합니다.

### 9.2 새 Component 추가

transform만 필요하면 `USceneComponent`, 렌더링이 필요하면 `UPrimitiveComponent` 또는 기존 mesh component 계층에서 시작합니다.

검토 항목:

- 객체 생성은 `NewObject`를 사용하는가?
- actor owner와 attachment가 올바르게 연결되는가?
- `Register`/`Unregister`가 대칭인가?
- `BeginPlay`/`EndPlay`가 대칭인가?
- 렌더 component라면 scene의 렌더 목록에 들어가는가?
- serialization에서 부모 구현을 호출하는가?
- 파괴 후 scene/editor가 dangling pointer를 보유하지 않는가?

### 9.3 새 에셋 추가

기존 `.example` 파일을 복사해 해당 타입의 JSON을 작성합니다. 예를 들어 material은 대략 다음 형태입니다.

```json
{
  "Version": 1,
  "Name": "Example_Material",
  "AssetType": "Material",
  "UPipelineID": "Pipeline/Example_Pipeline.json",
  "UTextureID": "Texture/Example_Texture.json",
  "TextureSampler": {
    "FilterMode": "Bilinear",
    "WrapMode": "Wrap"
  }
}
```

Asset ID는 파일명만이 아니라 `Content` 기준 상대 경로입니다. 참조 문자열의 대소문자, slash, 확장자를 기존 파일과 일관되게 유지합니다.

텍스처 원본을 추가했다면 수동 빌드 스크립트 또는 전체 빌드를 실행해 DDS 변환과 출력 폴더 복사를 수행합니다.

### 9.4 새 Shader/Pipeline 추가

1. `Shader` 아래에 `*VS.hlsl` 또는 `*PS.hlsl`을 만듭니다.
2. vertex shader entry는 `MainVS`, pixel shader entry는 `MainPS`를 사용합니다.
3. 공통 상수 구조는 `Shader/Constants.hlsli`와 C++의 `ShaderConstants.h`를 함께 확인합니다.
4. project를 재생성하고 빌드해 `.cso` 생성을 확인합니다.
5. `Content/Pipeline`에 pipeline JSON을 작성합니다.
6. 필요한 material JSON에서 pipeline과 texture asset ID를 연결합니다.

C++과 HLSL 양쪽의 constant buffer layout은 필드 순서, 크기, alignment가 일치해야 합니다. 한쪽만 수정하면 화면 깨짐이나 GPU 디버그 경고가 발생할 수 있습니다.

### 9.5 새 ImGui 패널 추가

1. `Source/Editor/UI/Imgui`에 `FImgui...Window` 타입을 추가합니다.
2. 패널 자체 상태와 scene/editor 상태를 구분합니다.
3. `FEditorApplication`이 소유하도록 연결합니다.
4. `Tick`의 적절한 위치에서 `Process`를 호출합니다.
5. View 메뉴 등에서 열고 닫는 경로를 제공합니다.
6. 선택 객체를 보유해야 한다면 raw pointer 수명과 scene 교체 시 초기화를 점검합니다.

---

## 10. 코딩 규칙

현재 저장소의 `.editorconfig`와 `.clang-format`에서 확인되는 기준입니다.

- 텍스트 인코딩: UTF-8
- 줄 끝: CRLF
- 파일 끝 newline 추가
- trailing whitespace 제거
- C/C++/HLSL 들여쓰기: tab, 폭 4
- Allman brace style
- pointer/reference 기호는 타입 쪽에 배치
- include 자동 정렬은 하지 않음
- 긴 줄을 formatter가 강제로 나누지 않음
- C++ source는 미리 컴파일 헤더 `pch.h`가 강제 include됨
- ThirdParty ImGui cpp는 warning/PCH/force-include 예외 처리됨

전체 또는 변경 파일 포맷에는 `Scripts/ClangFormat.ps1`을 확인해 사용합니다. 포맷만을 위한 광범위한 변경은 실제 기능 변경과 분리하는 편이 리뷰하기 좋습니다.

### 이름에서 읽을 수 있는 역할

프로젝트는 Unreal 스타일 접두사를 많이 사용합니다.

- `F`: 값 타입, manager, renderer, utility 구조체/클래스
- `U`: `UObject` 계열 객체나 asset/component/scene
- `A`: actor
- `T`: template container 또는 smart pointer alias
- `E`: enum
- `b`: bool 멤버/변수

이는 절대 규칙이라기보다 기존 코드를 읽는 데 유용한 관례입니다. 새 타입은 주변 디렉터리의 관례를 우선합니다.

---

## 11. 디버깅 가이드

### 11.1 애플리케이션이 시작되지 않음

확인 순서:

1. Visual Studio 구성과 플랫폼이 올바른지 확인합니다.
2. DirectXTK submodule이 실제로 채워져 있는지 확인합니다.
3. 프로젝트를 Premake로 다시 생성합니다.
4. 빌드 Output의 첫 번째 오류부터 확인합니다.
5. fatal error message box와 Visual Studio Output 로그를 함께 확인합니다.

`wWinMain`은 `std::exception`을 잡아 로그, debugger output, message box에 오류를 남깁니다.

### 11.2 에셋이 보이지 않음

확인 순서:

1. 실행 중인 exe 옆에 `Content`가 있는지 확인합니다.
2. 해당 JSON이 유효한 문법인지 확인합니다.
3. `Version`이 1인지 확인합니다.
4. `AssetType` 철자가 지원 목록과 일치하는지 확인합니다.
5. 참조하는 asset ID가 `Content` 기준 상대 경로인지 확인합니다.
6. 이미지라면 DDS가 생성되었는지 확인합니다.
7. Output에서 parse, version mismatch, duplicate name 로그를 검색합니다.

애셋만 변경했다면 `RunBuildScript_Debug.bat`을 실행하는 것이 가장 빠른 첫 조치입니다.

### 11.3 객체는 생성됐지만 렌더되지 않음

- actor가 현재 scene에 spawn되었는지 확인합니다.
- actor/component가 initialize되었는지 확인합니다.
- root component와 transform이 유효한지 확인합니다.
- primitive component가 scene에 register되었는지 확인합니다.
- mesh/material/pipeline 포인터가 유효한지 확인합니다.
- show flag와 view mode 때문에 숨겨지지 않았는지 확인합니다.
- 카메라 frustum, scale, 위치를 확인합니다.

### 11.4 Scene 로드가 실패함

- 파일이 존재하고 읽을 수 있는지 확인합니다.
- 최상위 `Version`이 2인지 확인합니다.
- `Scene` 항목이 있는지 확인합니다.
- 저장된 `Type` 문자열이 `UClass`에 등록된 이름인지 확인합니다.
- 새 타입에 `IMPLEMENT_UCLASS`가 빠지지 않았는지 확인합니다.
- 구버전 sample scene을 현재 포맷으로 오해하지 않았는지 확인합니다.

### 11.5 셰이더 오류

- 파일 이름이 `VS.hlsl` 또는 `PS.hlsl` 패턴을 만족하는지 확인합니다.
- entry point가 각각 `MainVS`, `MainPS`인지 확인합니다.
- pipeline JSON의 `.cso` 경로를 확인합니다.
- HLSL/C++ constant layout을 대조합니다.
- 빌드된 `Content/Shader`가 exe 옆으로 복사됐는지 확인합니다.

### 11.6 UI 또는 입력이 이상함

Win32 message는 먼저 ImGui handler에 전달됩니다. ImGui가 메시지를 소비하면 engine input 처리로 이어지지 않을 수 있습니다. 또한 포커스를 잃을 때 마우스 버튼 상태를 강제로 release하여 stuck input을 방지합니다. 문제를 추적할 때 아래 순서를 봅니다.

```text
WindowCallback
  -> ImGui_ImplWin32_WndProcHandler
  -> FInputManager event
  -> FInputManager::BeginFrame
  -> viewport/controller/editor 소비
```

---

## 12. 변경 전후 체크리스트

### 구현 전

- 변경이 Runtime, Editor, Content, Shader 중 어디의 책임인지 정했습니다.
- 비슷한 기존 구현을 하나 이상 찾았습니다.
- 객체 생성 및 소유자를 확인했습니다.
- serialization 호환성에 영향이 있는지 확인했습니다.
- asset dependency/load order에 영향이 있는지 확인했습니다.

### 구현 후

- Debug x64 빌드에 성공합니다.
- 실행과 종료가 정상입니다.
- 새 기능의 생성/편집/삭제 흐름을 시험했습니다.
- scene 저장 후 재실행하여 load를 시험했습니다.
- 새 asset이 출력 `Content`에서도 존재합니다.
- 새 shader가 `.cso`로 만들어졌습니다.
- warning과 로그를 확인했습니다.
- formatter 적용으로 무관한 파일이 대량 변경되지 않았습니다.
- 생성된 프로젝트/빌드 산출물을 실수로 commit하지 않았습니다.

### 리뷰 요청 시 설명하면 좋은 내용

- 무엇을 바꿨는가
- 왜 이 계층에서 바꿨는가
- 객체와 GPU resource의 소유자는 누구인가
- 저장 포맷 또는 asset schema가 변했는가
- 직접 수행한 테스트는 무엇인가
- 알려진 제약과 후속 작업은 무엇인가

---

## 13. 추천 코드 읽기 순서

처음부터 모든 파일을 읽을 필요는 없습니다. 관심사별로 다음 경로를 권장합니다.

### 전체 흐름을 알고 싶을 때

1. `Source/main.cpp`
2. `Source/Editor/Application/FEditorApplication.cpp`
3. `Source/Editor/Core/FEditor.cpp`
4. `Source/Runtime/Engine/USceneManager.cpp`
5. `Source/Runtime/Engine/UScene.cpp`
6. `Source/Runtime/Engine/FRenderView.cpp`
7. `Source/Runtime/Rendering/FRenderer.cpp`

### 객체와 씬 저장을 알고 싶을 때

1. `Source/Runtime/CoreUObject/UObject.h`
2. `Source/Runtime/CoreUObject/UObjectGlobals.h`
3. `Source/Runtime/CoreUObject/UClass.h`
4. `Source/Runtime/Actors/AActor.cpp`
5. `Source/Runtime/CoreUObject/USceneComponent.cpp`
6. `Source/Runtime/Engine/FArchive.cpp`
7. `Source/Runtime/Engine/UScene.cpp`

### 에셋과 렌더링을 알고 싶을 때

1. `Source/Runtime/Resource/FResourceLoader.cpp`
2. `Source/Runtime/Asset/FAssetRegistry.cpp`
3. `Source/Runtime/Rendering/FRenderResourceLibrary.cpp`
4. `Source/Runtime/Rendering/FRenderQueue.h`
5. `Source/Runtime/Engine/FRenderView.cpp`
6. `Content/Pipeline/*.json`
7. `Shader/*.hlsl`

### 에디터 기능을 추가하고 싶을 때

1. `Source/Editor/Application/FEditorApplication.cpp`
2. `Source/Editor/UI/Imgui/FImguiToolBar.cpp`
3. `Source/Editor/UI/Imgui/FImguiEditorViewportWindow.cpp`
4. `Source/Editor/UI/Imgui/FImguiWorldOutliner.cpp`
5. `Source/Editor/UI/Imgui/FImguiPropertyWindow.cpp`
6. `Source/Editor/Core/FEditor.cpp`

---

## 14. 첫날 실습 과제 예시

온보딩이 끝났는지 확인하기 위한 작은 실습입니다.

1. 프로젝트를 Debug x64로 빌드하고 실행합니다.
2. 에디터에서 cube actor를 생성합니다.
3. transform을 변경하고 `.Scene` 파일로 저장합니다.
4. 새 scene을 만든 다음 방금 저장한 scene을 다시 불러옵니다.
5. View 메뉴에서 2 × 2 layout을 선택합니다.
6. `stat fps`, `stat memory`, `stat none`을 실행합니다.
7. 기존 texture 하나를 교체하고 수동 build script로 Content를 갱신합니다.
8. `ACubeActor`가 생성되어 render component로 이어지는 코드 경로를 메모합니다.

이 실습이 모두 되면 빌드 파이프라인, 에셋 배포, 씬 수명, UI 조작의 기본 흐름을 경험한 것입니다.

---

## 15. 현재 코드에서 알아둘 주의점

- `README.md`는 현재 내용이 없으므로 실질적인 진입 문서는 `Docs`를 사용해야 합니다.
- 기존 `INTRODUCTION.md`와 `SETTING_ENVIRONMENT.md`의 내용은 현재 거의 동일합니다.
- 프로젝트 이름(`MyEngine`)과 제품/창 이름(`OIIAII`)이 혼용됩니다.
- Scene schema는 현재 version 2지만 저장소에 version 1 샘플이 남아 있을 수 있습니다.
- F5/F6/F7 scene request flag는 선언/설정되지만 현재 실제 동작과 연결되지 않은 것으로 보입니다.
- 메뉴의 `Import Import`는 OBJ import 기능으로 보이며 UI 문구 정리가 필요합니다.
- `PostBuild`는 대상 `Content` 디렉터리를 제거한 후 전체 복사합니다. 출력 폴더의 Content를 직접 수정하면 다음 빌드에서 사라집니다.
- Runtime이 읽는 Content 경로는 exe 기준입니다. IDE의 working directory만 바꾸어 해결하려 하지 마십시오.
- 에셋 registry는 중복 ID를 허용하지 않습니다.
- Scene 및 editor가 raw pointer를 여러 곳에서 참조하므로 파괴/교체 시 참조 해제를 점검해야 합니다.

이 항목들은 프로젝트 결함 목록이라기보다, 새 팀원이 시간을 잃기 쉬운 지점을 기록한 것입니다. 코드가 변경되면 함께 갱신합니다.

---

## 16. 용어집

| 용어 | 이 프로젝트에서의 의미 |
|---|---|
| Actor | scene에 배치되는 논리 객체 |
| Component | actor의 transform, rendering 등 기능 단위 |
| Primitive Component | render data를 제공하는 component |
| Scene | actor collection과 render component 목록을 관리하는 객체 |
| Asset | JSON metadata로 로드되어 registry에 등록되는 리소스 |
| Asset ID | `Content` 기준 JSON 상대 경로 또는 `#` 내부 ID |
| Pipeline | shader와 rasterizer/blend/depth 상태의 묶음 |
| Render View | scene을 특정 camera/view 설정으로 그리는 상위 렌더 흐름 |
| Viewport | 에디터 창 안에서 scene을 보는 개별 영역 |
| Gizmo | 선택 객체의 이동/회전/크기 조절 도구 |
| UClass | 문자열 타입 조회, 생성, 상속 판별을 위한 런타임 타입 정보 |
| FArchive | 객체와 JSON 사이의 직렬화 인터페이스 |

---

## 17. 문서 유지보수 규칙 제안

이 문서는 예시이므로 팀에 맞게 자유롭게 줄이거나 나누어도 됩니다. 다만 아래 항목은 코드 변경과 함께 갱신하는 것을 권장합니다.

- 빌드 도구와 Visual Studio 버전
- configuration/platform/output 경로
- 초기화와 프레임 순서
- asset 및 scene schema version
- 지원 asset type과 load order
- 사용자 단축키와 console command
- 폴더 구조
- 알려진 제약

문서의 명령은 새 PC에서 복사해 실행할 수 있어야 하고, 추측성 설명에는 명시적으로 `예정`, `제안`, `추정`을 붙이는 것이 좋습니다. 가장 중요한 것은 문서의 양이 아니라, 새 팀원이 막히는 지점을 실제 코드와 연결해 설명하는 것입니다.

---

## 부록 A. 자주 쓰는 명령 모음

```powershell
# submodule 초기화
git submodule update --init --recursive

# Visual Studio 프로젝트 생성
.\GenerateProjects.bat

# 또는 Premake 직접 실행
premake5 vs2026

# 코드 변경 없이 Debug Content만 갱신
.\RunBuildScript_Debug.bat

# 코드 변경 없이 Release Content만 갱신
.\RunBuildScript_Release.bat

# 현재 변경 확인
git status --short
```

## 부록 B. 빠른 문제 분류

| 증상 | 가장 먼저 볼 곳 |
|---|---|
| compile/link 실패 | `premake5.lua`, submodule, 첫 compiler error |
| 실행 직후 종료 | `main.cpp` catch message, VS Output |
| texture가 갱신 안 됨 | prebuild 결과와 exe 옆 `Content` |
| asset JSON을 못 찾음 | asset ID와 실행 파일 기준 Content 경로 |
| scene을 못 읽음 | schema version과 등록된 `Type` 문자열 |
| actor가 안 보임 | component Register, mesh/material, camera |
| ImGui 입력만 되고 scene 입력 안 됨 | ImGui handler의 message consume 여부 |
| 새 cpp가 빌드에 안 들어감 | Premake 재실행 |
| shader가 반영 안 됨 | 파일 suffix, entry point, `.cso`, Content 복사 |


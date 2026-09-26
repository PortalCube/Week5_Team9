# Oiiaii 엔진 살펴보기

마지막 업데이트: 5주차 (2026-09-24)

[온보딩 문서로 돌아가기](./ONBOARDING.md)

이 문서에서는 Oiiaii 엔진을 누구나 쉽게 이해할 수 있도록 간단하게 살펴봅니다.

## 스펙

| 항목            | 설명               |
| --------------- | ------------------ |
| 컴파일러        | MSVC v145          |
| 플랫폼          | Windows Only       |
| IDE             | Visual Studio 2026 |
| C++ 표준        | C++20              |
| 그래픽스 백엔드 | DirectX11          |
| 빌드 시스템     | Premake5           |

## 외부 라이브러리

| 항목                                                | 설명                        |
| --------------------------------------------------- | --------------------------- |
| ImGui                                               | 에디터 UI 구현에 사용       |
| [DirectXTK](https://github.com/microsoft/directxtk) | DDS 포맷 Decoding, Encoding |
| [nlohmann/json](https://github.com/nlohmann/json)   | JSON 직렬화 라이브러리      |
| [mINI](https://github.com/metayeti/mINI)            | INI 직렬화 라이브러리       |
| mINI                                                | INI 파서                    |

## 엔진 구조

### 메인 게임 루프

### 렌더링 루프

### Scene 생명 주기 루프

### Editor 루프

## 특징

## 장점

-

## 단점

### 낮은 가독성

- 저희도 가독성이 나쁜 파일들이 꽤 있습니다. 대표적으로 아래 파일들이 있습니다.
- FRenderer.cpp (1200줄)

### Renderer 계층의 분리 미비

FRenderer는 원칙적으로 다른 계층이 알지 못해야합니다. (현재 설계는 Renderer의 기능을 쓸 필요가 있다면, RenderView를 사용해야만 하도록 되어있습니다.)

그러나 현재는 FGrid, FGizmo에서 FRenderer를 받아서 사용하고 있는 부분이 있습니다.

언젠가는 FRenderView에서 이런 부분을 받아가도록 수정해야합니다.

### 인게임/에디터 분리 미비

현재 Editor와 Runtime 폴더는 분리는 되어있으나... 실제 코드는 Editor와 Runtime 코드가 딱히 분리되어 있진 않습니다.

현재는 인게임 모드를 따로 분리할 계획이라면 구조적 리팩토링이 필수적으로 발생합니다.

# 테스트 살펴보기

마지막 업데이트: 5주차 (2026-09-24)

[온보딩 문서로 돌아가기](./ONBOARDING.md)

이 문서에서는 Oiiaii 엔진에서 기능을 테스트하는 방법에 대해 알아봅니다.

## 테스트 실행해보기

OiiaiiEngine은 [언리얼 엔진의 low-level 테스트](https://dev.epicgames.com/documentation/unreal-engine/low-level-tests-in-unreal-engine)에서 사용되는 Catch2 프레임워크를 사용합니다.

OiiaiiEngine.Test 프로젝트에서 단위 테스트나 통합 테스트 등을 직접 실행해 볼 수 있는데, 여기서는 두가지 방법을 설명합니다.

### 그냥 실행해보기

그냥 프로젝트를 직접 빌드 후 실행해서 콘솔 창에서 결과를 확인할 수 있습니다. Test 프로젝트를 우클릭해서 실행하면 됩니다.

https://github.com/user-attachments/assets/e1ac2e66-9b79-47c8-8bab-82a19ee69db4

<video controls src="./Resources/Run_Test_1.mp4" width="720"></video>

### 테스트 탐색기에서 실행하기

조금 귀찮지만 화려한 UI로 결과를 확인할 수 있습니다.

https://github.com/user-attachments/assets/1237fcb9-e626-4a05-80c7-e7161b8e7f1b

<video controls src="./Resources/Run_Test_2.mp4" width="720"></video>

참고로 중간에 "다시 빌드"를 누르는 과정은 스킵하실 수 있는데, 만약 테스트가 하나도 안보이면서 버튼이 비활성화 되었다면 다시 빌드를 실행해주세요.

## 테스트 추가하기

실제 예시는 Test/WindowsUtil_Test.cpp 파일을 참고해주세요.

Test 폴더에 테스트하려는 파일에 "\_Test" 접미사를 붙여서 새 cpp 파일을 만들고, 테스트 코드를 작성해주세요.

TEST_CASE 하위에 SECTION을 추가하고, CHECK, CHECK_FALSE, CHECK_THROWS 등으로 assert 하듯이 값을 테스트하시면 됩니다.

Catch2의 기능과 사용 방법에 대해서는 아래의 참고 자료를 확인해주세요.

- [Catch2 Tutorial](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md)
- [Catch2 레퍼런스](https://github.com/catchorg/Catch2/blob/devel/docs/Readme.md)

## 태그 규칙

대충 아래의 태그 규칙이 존재합니다.

- 테스트 유형
    - [unit]: 유닛 테스트
    - [integration]: 통합 테스트
    - [e2e]: E2E 테스트

- 코드 분야
    - [actor] : AActor
    - [component] : UComponent
    - [uobject] : UObject
    - [math] : FVector, FMatrix, ...
    - [utility] : WindowsUtil, EngineUtil
    - [imgui] : Imgui 관련
    - [archive] : FArchive 관련
    - [renderer] : FRenderer 관련

## 테스트는 언제 붙여야 하는가?

모든 코드에 테스트를 붙여서 커버리지 100%를 달성하면 좋겠지만, 저희는 과제의 기능 구현 및 버그 수정이 가장 우선이므로 테스트는 우선순위의 뒷편에 두어도 괜찮습니다.

대신 아래의 코드에는 테스트를 적극적으로 붙이는걸 고려해보세요.

- 엔진의 핵심 코드로, 자주 쓰이면서 버그가 발생하면 치명적인 코드

- 기능 추가/수정을 할 때마다 버그가 자주 발생하는 코드

- 누가 봐도 유닛 테스트를 붙이기 쉬워보이는 코드 (ex. FArchive.cpp, FVector.cpp)

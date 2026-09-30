#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Engine/FEngine.h"

// 엔진의 전역 변수를 담는 네임스페이스입니다.
namespace Globals
{
	// 엔진 이름
	constexpr FStringView EngineName = "Oiiaii";
	constexpr FStringView EngineWindowClass = "OiiaiiClass";

	// 초기 윈도우 사이즈
	constexpr uint32 WindowWidth = 1600;
	constexpr uint32 WindowHeight = 900;

	// 이번 Tick 이후로 애플리케이션이 종료되어야 하는지 여부
	inline bool bIsRequestingExit = false;

	// 윈도우 크기 변경 요청과 변경될 크기
	inline bool bIsRequestingResize = false;
	inline uint32 ResizeWidth = 0u;
	inline uint32 ResizeHeight = 0u;

	inline bool bEnableRenderSort = true;
	inline bool bEnableBatchTransform = true;
	inline bool bSortTest = false;
	inline bool bUseSIMDCulling = true;

	// LOD 설정. ForcedLOD가 0 이상이면 화면 크기와 상관없이 해당 LOD로 고정한다.
	inline bool bEnableLOD = true;
	inline int32 ForcedLOD = -1;

	// LOD 디버그. 색상 표시를 켜면 LOD마다 다른 색으로 칠한다.
	constexpr uint32 MaxDebugLODCount = 4;
	inline bool bShowLODColor = false;
	inline uint32 LODDrawCounts[MaxDebugLODCount] = {};	// 마지막으로 그린 뷰의 LOD별 컴포넌트 수

	// 컬링 설정
	inline bool bEnableFrustumCulling = true;
	inline uint32 FrustumVisibleCount = 0;   // 마지막으로 그린 뷰에서 Frustum을 통과한 수 (표시용)

	inline bool bEnableOcclusionCulling = false;
	inline int32 OccluderBudget = 1024;        // Occluder로 쓸 가까운 오브젝트 수
	inline int32 OcclusionBufferWidth = 512;   // CPU 깊이 버퍼 가로 해상도
	inline bool bIncludeOccluderCull = false;  // Occluder 자신도 판정 대상에 포함
	inline bool bRequestOcclusionOracle = false; // 버튼: 다음 뷰에서 오라클 1회 (한 프레임 멈춤)
	inline bool bRequestOcclusionDump = false;   // 버튼: 다음 프레임 깊이 버퍼 BMP 저장
	inline uint32 OccludedCount = 0;             // 표시용
};

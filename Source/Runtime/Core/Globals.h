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
};

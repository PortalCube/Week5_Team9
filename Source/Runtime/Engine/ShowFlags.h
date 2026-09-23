#pragma once

#include "Runtime/Core/IntTypes.h"

// 렌더링 모드
enum class EViewModeIndex : uint8 {
  VMI_Lit,
  VMI_Unlit,
  VMI_Wireframe,
};

// 렌더링 쇼 플래그
enum class EEngineShowFlags : uint64 {
	SF_Primitives = 1ULL << 0,
	SF_BillboardText = 1ULL << 1,
	SF_Grid = 1ULL << 2,

};

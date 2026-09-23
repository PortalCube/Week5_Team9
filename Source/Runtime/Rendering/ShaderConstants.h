#pragma once

#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"

// Register = b0
struct FFrameConstants {
	float Time;
	float DeltaTime;
	FVector2 Padding;
};
static_assert(sizeof(FFrameConstants) % 16 == 0);

// Register = b1
struct FViewConstants {
	FMatrix VP;
	FVector2 ViewportSize;
	FVector2 Padding;
};
static_assert(sizeof(FViewConstants) % 16 == 0);

// Register = b2
struct FObjectConstants {
  FMatrix MVP;
  FVector4 Color{0.0f, 0.0f, 0.0f, 0.0f};
  FVector2 UVScale{1.0f, 1.0f};
  FVector2 UVOffset{0.0f, 0.0f};
  FMatrix World = FMatrix::GetIdentity();
  float DisableShading = 0.0f;
  FVector Padding;
};
static_assert(sizeof(FObjectConstants) % 16 == 0);


// Register = b3
//struct FShaderConstants {
//
//};
//static_assert(sizeof(FShaderConstants) % 16 == 0);


// Register = b2 / ObjectConstants Override
struct FGridConstants {
  FMatrix MVP;
  FMatrix World;
  float CellSize;
  FVector Padding;
};

static_assert(sizeof(FGridConstants) % 16 == 0);

// Register = b2 / ObjectConstants Override
// LINE_LIST 기반 에디터 그리드용 상수 버퍼
struct FGridLineConstants {
  FMatrix MVP;
  FVector CameraPosition;
  float FadeStartDistance;
  float FadeEndDistance;
  FVector Padding;
};

static_assert(sizeof(FGridLineConstants) % 16 == 0);

// 나중에 수정 필요
// Register = b4
struct FLightConstants {
  // 기본 조명 파라미터
  FVector LightDirection{-0.5f, -0.5f, -1.0f};
  float Intensity = 1.0f;

  FVector LightColor{1.0f, 1.0f, 1.0f};
  float AmbientIntensity = 0.2f;
};

static_assert(sizeof(FLightConstants) % 16 == 0);

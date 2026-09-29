#pragma once

#include "Runtime/Core/TArray.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/Vertices.h"

// meshoptimizer로 원본 메시를 단순화해 LOD 메시 데이터를 만든다.
namespace MeshLODBuilder
{
	struct FLODSetting
	{
		float TriangleRatio = 0.5f;	// 원본 대비 목표 삼각형 비율
		float MaxError = 0.02f;		// 허용 오차 (메시 크기 대비 비율). 이 오차를 넘으면 목표 비율 전에 멈춘다.
		float ScreenSize = 0.5f;	// 화면 점유율이 이 값보다 작아지면 이 LOD로 전환
	};

	struct FLODMeshData
	{
		TArray<FVertexData> Vertices;
		TArray<uint32> Indices;
		TArray<FMeshSection> Sections;
	};

	// 정적 메시에 기본으로 적용하는 LOD 설정 (LOD1부터)
	const TArray<FLODSetting>& GetDefaultSettings();

	// 섹션별로 단순화한다. 섹션 경계는 잠가서 머티리얼 사이에 틈이 생기지 않게 한다.
	void BuildLOD(
		const TArray<FVertexData>& Vertices,
		const TArray<uint32>& Indices,
		const TArray<FMeshSection>& Sections,
		const FLODSetting& Setting,
		FLODMeshData& OutData);
}

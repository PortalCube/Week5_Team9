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

	// GPU 효율을 위해 메시를 제자리에서 최적화한다. 삼각형의 모양은 바뀌지 않는다.
	// 중복 정점 병합 → 퇴화/중복 삼각형 제거 → (섹션별) 정점 캐시 → (섹션별) 오버드로우 → 정점 fetch 순서.
	// 섹션의 StartIndex/IndexCount는 결과에 맞게 갱신되고, 섹션 개수와 순서는 유지된다.
	void OptimizeMesh(
		TArray<FVertexData>& Vertices,
		TArray<uint32>& Indices,
		TArray<FMeshSection>& Sections);

	// 섹션별로 단순화한다. 섹션 경계는 잠가서 머티리얼 사이에 틈이 생기지 않게 한다.
	// 결과는 OptimizeMesh까지 적용된 상태다.
	void BuildLOD(
		const TArray<FVertexData>& Vertices,
		const TArray<uint32>& Indices,
		const TArray<FMeshSection>& Sections,
		const FLODSetting& Setting,
		FLODMeshData& OutData);
}

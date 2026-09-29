#include "MeshLODBuilder.h"

#include "meshoptimizer.h"

const TArray<MeshLODBuilder::FLODSetting>& MeshLODBuilder::GetDefaultSettings()
{
	static const TArray<FLODSetting> Settings
	{
		{ .TriangleRatio = 0.5f,  .MaxError = 0.02f, .ScreenSize = 0.5f  },
		{ .TriangleRatio = 0.25f, .MaxError = 0.05f, .ScreenSize = 0.2f },
		{ .TriangleRatio = 0.1f,  .MaxError = 0.1f,  .ScreenSize = 0.03f  },
	};
	return Settings;
}

void MeshLODBuilder::BuildLOD(
	const TArray<FVertexData>& Vertices,
	const TArray<uint32>& Indices,
	const TArray<FMeshSection>& Sections,
	const FLODSetting& Setting,
	FLODMeshData& OutData)
{
	OutData = {};
	if (Vertices.empty() || Indices.empty()) { return; }

	// UV(u, v)와 노멀(nx, ny, nz)은 FVertexData에서 연속된 float 5개다.
	// 이 값들이 크게 바뀌는 붕괴는 비용을 높여서 텍스처와 셰이딩이 무너지지 않게 한다.
	constexpr size_t AttributeCount = 5;
	constexpr float AttributeWeights[AttributeCount] = { 0.5f, 0.5f, 0.25f, 0.25f, 0.25f };

	TArray<uint32> Simplified;
	OutData.Indices.reserve(Indices.size());

	for (const FMeshSection& Section : Sections)
	{
		// 머티리얼 슬롯 순서를 맞추기 위해 빈 섹션도 그대로 유지한다.
		if (Section.IndexCount < 3)
		{
			OutData.Sections.push_back({ Section.SectionName, static_cast<uint32>(OutData.Indices.size()), 0 });
			continue;
		}

		const size_t TargetIndexCount = static_cast<size_t>(Section.IndexCount * Setting.TriangleRatio) / 3 * 3;

		// meshopt는 결과 버퍼로 원본 인덱스 수만큼의 공간을 요구한다.
		Simplified.resize(Section.IndexCount);
		const size_t NewIndexCount = meshopt_simplifyWithAttributes(
			Simplified.data(),
			Indices.data() + Section.StartIndex, Section.IndexCount,
			&Vertices[0].x, Vertices.size(), sizeof(FVertexData),
			&Vertices[0].u, sizeof(FVertexData), AttributeWeights, AttributeCount,
			nullptr,
			TargetIndexCount, Setting.MaxError, meshopt_SimplifyLockBorder, nullptr);

		FMeshSection NewSection = Section;
		NewSection.StartIndex = static_cast<uint32>(OutData.Indices.size());
		NewSection.IndexCount = static_cast<uint32>(NewIndexCount);
		OutData.Indices.insert(OutData.Indices.end(), Simplified.begin(), Simplified.begin() + NewIndexCount);
		OutData.Sections.push_back(NewSection);
	}

	// 단순화 후 참조되지 않는 정점을 제거하고, 인덱스 순서대로 정점을 재배치한다.
	OutData.Vertices.resize(Vertices.size());
	const size_t UniqueVertexCount = meshopt_optimizeVertexFetch(
		OutData.Vertices.data(),
		OutData.Indices.data(), OutData.Indices.size(),
		Vertices.data(), Vertices.size(), sizeof(FVertexData));
	OutData.Vertices.resize(UniqueVertexCount);
}

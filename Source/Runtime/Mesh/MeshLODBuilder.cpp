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

	// 단순화 결과는 원본 정점 버퍼를 참조하므로, 복사한 뒤 최적화하면서 쓰지 않는 정점을 제거한다.
	OutData.Vertices = Vertices;
	OptimizeMesh(OutData.Vertices, OutData.Indices, OutData.Sections);
}

void MeshLODBuilder::OptimizeMesh(
	TArray<FVertexData>& Vertices,
	TArray<uint32>& Indices,
	TArray<FMeshSection>& Sections)
{
	if (Vertices.empty() || Indices.empty()) { return; }

	// 1) 바이트 단위로 완전히 같은 정점을 하나로 합친다.
	{
		TArray<uint32> Remap(Vertices.size());
		const size_t UniqueCount = meshopt_generateVertexRemap(
			Remap.data(), Indices.data(), Indices.size(),
			Vertices.data(), Vertices.size(), sizeof(FVertexData));

		TArray<FVertexData> Unique(UniqueCount);
		meshopt_remapVertexBuffer(Unique.data(), Vertices.data(), Vertices.size(), sizeof(FVertexData), Remap.data());
		meshopt_remapIndexBuffer(Indices.data(), Indices.data(), Indices.size(), Remap.data());
		Vertices = std::move(Unique);
	}

	// 2~4) 섹션은 각각 드로우 범위이므로 섹션 단위로 처리하고, 결과를 앞에서부터 다시 채운다.
	TArray<uint32> Optimized;
	Optimized.reserve(Indices.size());
	TArray<uint32> Scratch;

	for (FMeshSection& Section : Sections)
	{
		const uint32 NewStart = static_cast<uint32>(Optimized.size());
		const uint32* SectionIndices = Indices.data() + Section.StartIndex;

		// 2) 위치가 겹쳐 면적이 0인 삼각형과, 같은 방향으로 중복된 삼각형을 제거한다.
		Scratch.resize(Section.IndexCount);
		const size_t FilteredCount = Section.IndexCount < 3 ? 0 : meshopt_filterIndexBuffer(
			Scratch.data(), SectionIndices, Section.IndexCount,
			&Vertices[0].x, Vertices.size(), sizeof(float) * 3, sizeof(FVertexData));

		if (FilteredCount > 0)
		{
			const size_t Offset = Optimized.size();
			Optimized.resize(Offset + FilteredCount);
			uint32* Target = Optimized.data() + Offset;

			// 3) 정점 셰이더 결과 캐시에 잘 맞도록 삼각형 순서를 바꾼다.
			meshopt_optimizeVertexCache(Target, Scratch.data(), FilteredCount, Vertices.size());

			// 4) 바깥쪽 면이 먼저 그려지도록 순서를 조정해 오버드로우를 줄인다.
			//    정점 캐시 효율은 최대 5%까지만 양보한다.
			meshopt_optimizeOverdraw(Scratch.data(), Target, FilteredCount,
				&Vertices[0].x, Vertices.size(), sizeof(FVertexData), 1.05f);
			std::copy(Scratch.begin(), Scratch.begin() + FilteredCount, Target);
		}

		Section.StartIndex = NewStart;
		Section.IndexCount = static_cast<uint32>(FilteredCount);
	}

	Indices = std::move(Optimized);
	if (Indices.empty()) { return; }

	// 5) 인덱스가 처음 등장하는 순서대로 정점을 재배치해 메모리 읽기를 연속적으로 만든다.
	//    쓰이지 않는 정점도 여기서 제거된다.
	TArray<FVertexData> Fetched(Vertices.size());
	const size_t UsedCount = meshopt_optimizeVertexFetch(
		Fetched.data(), Indices.data(), Indices.size(),
		Vertices.data(), Vertices.size(), sizeof(FVertexData));
	Fetched.resize(UsedCount);
	Vertices = std::move(Fetched);
}

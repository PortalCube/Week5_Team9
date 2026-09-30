#include "catch_amalgamated.hpp"

#include "Runtime/Mesh/MeshLODBuilder.h"
#include "meshoptimizer.h"

#include <cmath>
#include <numbers>

namespace
{
    // 위/아래 반구를 서로 다른 섹션으로 가진 UV 구를 만든다.
    void CreateSphere(uint32 Rings, uint32 Segments,
                      TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections)
    {
        for (uint32 r = 0; r <= Rings; ++r)
        {
            const float Theta = std::numbers::pi_v<float> * r / Rings;
            // 극점은 모든 정점의 위치가 바이트 단위로 같아야 한다.
            // float의 sin(pi)는 0이 아니고, 0 * 음수는 -0이 되므로 곱하지 않고 0을 넣는다.
            const bool bPole = (r == 0 || r == Rings);
            const float SinTheta = std::sin(Theta);
            for (uint32 s = 0; s <= Segments; ++s)
            {
                const float Phi = 2.0f * std::numbers::pi_v<float> * s / Segments;
                FVertexData Vertex{};
                Vertex.nx = bPole ? 0.0f : SinTheta * std::cos(Phi);
                Vertex.ny = bPole ? 0.0f : SinTheta * std::sin(Phi);
                Vertex.nz = std::cos(Theta);
                Vertex.x = Vertex.nx;
                Vertex.y = Vertex.ny;
                Vertex.z = Vertex.nz;
                Vertex.u = static_cast<float>(s) / Segments;
                Vertex.v = static_cast<float>(r) / Rings;
                OutVertices.push_back(Vertex);
            }
        }

        for (uint32 Half = 0; Half < 2; ++Half)
        {
            FMeshSection Section{ Half == 0 ? "Top" : "Bottom", static_cast<uint32>(OutIndices.size()), 0 };
            for (uint32 r = Half * Rings / 2; r < (Half + 1) * Rings / 2; ++r)
            {
                for (uint32 s = 0; s < Segments; ++s)
                {
                    const uint32 A = r * (Segments + 1) + s;
                    const uint32 B = A + Segments + 1;
                    OutIndices.insert(OutIndices.end(), { A, B, A + 1, A + 1, B, B + 1 });
                }
            }
            Section.IndexCount = static_cast<uint32>(OutIndices.size()) - Section.StartIndex;
            OutSections.push_back(Section);
        }
    }
}

TEST_CASE(
    "MeshLODBuilder",
    "[unit][mesh][lod]")
{
    TArray<FVertexData> Vertices;
    TArray<uint32> Indices;
    TArray<FMeshSection> Sections;
    CreateSphere(32, 64, Vertices, Indices, Sections);

    MeshLODBuilder::FLODMeshData LOD;
    MeshLODBuilder::BuildLOD(Vertices, Indices, Sections,
        { .TriangleRatio = 0.5f, .MaxError = 0.05f, .ScreenSize = 0.5f }, LOD);

    SECTION("triangle count is reduced")
    {
        CHECK(!LOD.Indices.empty());
        CHECK(LOD.Indices.size() % 3 == 0);
        CHECK(LOD.Indices.size() <= Indices.size() * 0.6);
    }

    SECTION("sections are preserved in order")
    {
        REQUIRE(LOD.Sections.size() == Sections.size());
        uint32 Expected = 0;
        for (size_t i = 0; i < Sections.size(); ++i)
        {
            CHECK(LOD.Sections[i].SectionName == Sections[i].SectionName);
            CHECK(LOD.Sections[i].StartIndex == Expected);
            CHECK(LOD.Sections[i].IndexCount > 0);
            Expected += LOD.Sections[i].IndexCount;
        }
        CHECK(Expected == LOD.Indices.size());
    }

    SECTION("vertices are compacted and indices are valid")
    {
        CHECK(LOD.Vertices.size() < Vertices.size());
        for (uint32 Index : LOD.Indices)
        {
            REQUIRE(Index < LOD.Vertices.size());
        }
    }
}

TEST_CASE(
    "MeshLODBuilder OptimizeMesh",
    "[unit][mesh][optimize]")
{
    TArray<FVertexData> Vertices;
    TArray<uint32> Indices;
    TArray<FMeshSection> Sections;
    CreateSphere(32, 64, Vertices, Indices, Sections);

    const TArray<FVertexData> SourceVertices = Vertices;
    const TArray<uint32> SourceIndices = Indices;
    const TArray<FMeshSection> SourceSections = Sections;

    MeshLODBuilder::OptimizeMesh(Vertices, Indices, Sections);

    SECTION("degenerate pole triangles are removed")
    {
        // 극점 링의 삼각형은 두 정점의 위치가 같아 면적이 0이다. 위/아래 극에 각각 Segments개.
        CHECK(Indices.size() == SourceIndices.size() - 2 * 64 * 3);
    }

    SECTION("sections stay contiguous and in order")
    {
        REQUIRE(Sections.size() == SourceSections.size());
        uint32 Expected = 0;
        for (size_t i = 0; i < Sections.size(); ++i)
        {
            CHECK(Sections[i].SectionName == SourceSections[i].SectionName);
            CHECK(Sections[i].StartIndex == Expected);
            Expected += Sections[i].IndexCount;
        }
        CHECK(Expected == Indices.size());
    }

    SECTION("indices are valid and vertices are ordered by first use")
    {
        uint32 NextNew = 0;
        for (uint32 Index : Indices)
        {
            REQUIRE(Index < Vertices.size());
            if (Index == NextNew) { ++NextNew; }
            REQUIRE(Index < NextNew);
        }
        CHECK(NextNew == Vertices.size());
    }

    SECTION("vertex cache efficiency improves")
    {
        const auto Before = meshopt_analyzeVertexCache(SourceIndices.data(), SourceIndices.size(), SourceVertices.size(), 16, 0, 0);
        const auto After = meshopt_analyzeVertexCache(Indices.data(), Indices.size(), Vertices.size(), 16, 0, 0);
        CHECK(After.acmr < Before.acmr);
    }
}

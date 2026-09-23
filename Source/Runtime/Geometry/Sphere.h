#pragma once

#include "Runtime/Rendering/Vertices.h"
#include <vector>
#include <cmath>
#include <numbers>

// 구체 정점 동적 계산 생성
inline std::vector<FVertexData> CreateSphereVertices(float radius = 0.5f, int slices = 20, int stacks = 20, bool bInward = false)
{
	std::vector<FVertexData> vertices;
	vertices.reserve(stacks * slices * 6);

	constexpr float pi = std::numbers::pi_v<float>;

	auto GetVertex = [radius, pi](float phi, float theta) -> FVertexData
	{
		float y = radius * cosf(phi);
		float r = radius * sinf(phi);
		float x = r * sinf(theta);
		float z = r * cosf(theta);

		float cr = (x / radius) * 0.5f + 0.5f;
		float cg = (y / radius) * 0.5f + 0.5f;
		float cb = (z / radius) * 0.5f + 0.5f;

		float nx = x / radius;
		float ny = y / radius;
		float nz = z / radius;

		float u = theta / (2.0f * pi);
		float v = phi / pi;

		return { x, y, z, cr, cg, cb, 1.0f, u, v, nx, ny, nz };
	};

	for (int i = 0; i < stacks; ++i)
	{
		float phi1 = pi * static_cast<float>(i) / static_cast<float>(stacks);
		float phi2 = pi * static_cast<float>(i + 1) / static_cast<float>(stacks);

		for (int j = 0; j < slices; ++j)
		{
			float theta1 = 2.0f * pi * static_cast<float>(j) / static_cast<float>(slices);
			float theta2 = 2.0f * pi * static_cast<float>(j + 1) / static_cast<float>(slices);

			FVertexData v1 = GetVertex(phi1, theta1);
			FVertexData v2 = GetVertex(phi1, theta2);
			FVertexData v3 = GetVertex(phi2, theta1);
			FVertexData v4 = GetVertex(phi2, theta2);

			if (!bInward)
			{
				vertices.push_back(v1);
				vertices.push_back(v3);
				vertices.push_back(v2);

				vertices.push_back(v2);
				vertices.push_back(v3);
				vertices.push_back(v4);
			}
			else
			{
				vertices.push_back(v1);
				vertices.push_back(v2);
				vertices.push_back(v3);

				vertices.push_back(v2);
				vertices.push_back(v4);
				vertices.push_back(v3);
			}
		}
	}

	return vertices;
}
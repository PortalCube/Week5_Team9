#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector4.h"
#include "Vertices.h"

#include <d3d11.h>
#include <wrl/client.h>

class FRenderer;
class FRenderPipeline;

// 라인 정점 구조체
struct FLineVertex {
	FVector Position;
	FVector4 Color;
};

// 라인 일괄 렌더러
class FLineBatcher final {
public:
	static constexpr uint32 MaxLines = 16384;
	static constexpr uint32 MaxVertices = MaxLines * 2;

	bool Initialize(ID3D11Device* Device);
	void Shutdown();

	// 단일 선 추가
	void DrawLine(const FVector& Start, const FVector& End, const FVector4& Color);

	// 바운딩 박스 와이어프레임 추가
	void DrawBoxCenterExtent(const FVector& Center, const FVector& Extent, const FVector4& Color);
	void DrawBoxMinMax(const FVector& Center, const FVector& Extent, const FVector4& Color);

	// Quad 와이어프레임 추가
	void DrawQuad(
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FVector4& Color
	);

	// 구체 와이어프레임 추가
	void DrawSphere(const FVector& Center, float Radius, const FVector4& Color, uint32 Segments = 16);

	// 수집된 선들을 일괄 렌더링하고 비움
	void Flush(ID3D11DeviceContext& Context,
		const TSharedPtr<FRenderPipeline>& Pipeline);

	// 대기 중인 정점 수
	[[nodiscard]] uint32 GetVertexCount() const { return static_cast<uint32>(LineVertices.size()); }

private:
	// 동적 라인 정점 버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> DynamicLineVertexBuffer;
	// 정점 리스트
	TArray<FVertexData> LineVertices;
};

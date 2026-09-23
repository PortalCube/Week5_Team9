#include "FLineBatcher.h"
#include "FRenderer.h"
#include "ShaderConstants.h"

#include <cmath>
#include <numbers>

bool FLineBatcher::Initialize(ID3D11Device* Device) {
	if (!Device) {
		return false;
	}

	// 동적 라인 정점 버퍼 생성
	D3D11_BUFFER_DESC VbDesc{
		.ByteWidth = static_cast<UINT>(sizeof(FVertexData) * MaxVertices),
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
	};

	HRESULT Result = Device->CreateBuffer(&VbDesc, nullptr, &DynamicLineVertexBuffer);
	if (FAILED(Result)) {
		return false;
	}

	LineVertices.reserve(MaxVertices);
	return true;
}

void FLineBatcher::Shutdown() {
	DynamicLineVertexBuffer.Reset();
	LineVertices.clear();
}

void FLineBatcher::DrawLine(const FVector& Start, const FVector& End, const FVector4& Color) {
	if (LineVertices.size() + 2 > MaxVertices) {
		return;
	}

	LineVertices.push_back(FVertexData{
		Start.X, Start.Y, Start.Z,
		Color.X, Color.Y, Color.Z, Color.W,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	});
	LineVertices.push_back(FVertexData{
		End.X, End.Y, End.Z,
		Color.X, Color.Y, Color.Z, Color.W,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	});
}

void FLineBatcher::DrawBoxCenterExtent(const FVector& Center, const FVector& Extent, const FVector4& Color) {
	const FVector Min = Center - Extent;
	const FVector Max = Center + Extent;
	DrawBoxMinMax(Min, Max, Color);
}

void FLineBatcher::DrawBoxMinMax(const FVector& Min, const FVector& Max, const FVector4& Color) {

	// 하단 사각형
	DrawLine(FVector{ Min.X, Min.Y, Min.Z }, FVector{ Max.X, Min.Y, Min.Z }, Color);
	DrawLine(FVector{ Max.X, Min.Y, Min.Z }, FVector{ Max.X, Max.Y, Min.Z }, Color);
	DrawLine(FVector{ Max.X, Max.Y, Min.Z }, FVector{ Min.X, Max.Y, Min.Z }, Color);
	DrawLine(FVector{ Min.X, Max.Y, Min.Z }, FVector{ Min.X, Min.Y, Min.Z }, Color);

	// 상단 사각형
	DrawLine(FVector{ Min.X, Min.Y, Max.Z }, FVector{ Max.X, Min.Y, Max.Z }, Color);
	DrawLine(FVector{ Max.X, Min.Y, Max.Z }, FVector{ Max.X, Max.Y, Max.Z }, Color);
	DrawLine(FVector{ Max.X, Max.Y, Max.Z }, FVector{ Min.X, Max.Y, Max.Z }, Color);
	DrawLine(FVector{ Min.X, Max.Y, Max.Z }, FVector{ Min.X, Min.Y, Max.Z }, Color);

	// 수직 기둥
	DrawLine(FVector{ Min.X, Min.Y, Min.Z }, FVector{ Min.X, Min.Y, Max.Z }, Color);
	DrawLine(FVector{ Max.X, Min.Y, Min.Z }, FVector{ Max.X, Min.Y, Max.Z }, Color);
	DrawLine(FVector{ Max.X, Max.Y, Min.Z }, FVector{ Max.X, Max.Y, Max.Z }, Color);
	DrawLine(FVector{ Min.X, Max.Y, Min.Z }, FVector{ Min.X, Max.Y, Max.Z }, Color);
}

void FLineBatcher::DrawQuad(
	const FVector& A,
	const FVector& B,
	const FVector& C,
	const FVector& D,
	const FVector4& Color
)
{
	DrawLine(A, B, Color);
	DrawLine(B, C, Color);
	DrawLine(C, D, Color);
	DrawLine(D, A, Color);
}

void FLineBatcher::DrawSphere(const FVector& Center, float Radius, const FVector4& Color, uint32 Segments) {
	if (Segments < 4) Segments = 4;
	const float Step = std::numbers::pi_v<float> * 2.0f / static_cast<float>(Segments);

	for (uint32 i = 0; i < Segments; ++i) {
		const float A0 = static_cast<float>(i) * Step;
		const float A1 = static_cast<float>(i + 1) * Step;

		const float C0 = std::cos(A0) * Radius;
		const float S0 = std::sin(A0) * Radius;
		const float C1 = std::cos(A1) * Radius;
		const float S1 = std::sin(A1) * Radius;

		// 평면별 원 그리기
		DrawLine(Center + FVector{ C0, S0, 0.0f }, Center + FVector{ C1, S1, 0.0f }, Color);
		DrawLine(Center + FVector{ 0.0f, C0, S0 }, Center + FVector{ 0.0f, C1, S1 }, Color);
		DrawLine(Center + FVector{ S0, 0.0f, C0 }, Center + FVector{ S1, 0.0f, C1 }, Color);
	}
}

void FLineBatcher::Flush(ID3D11DeviceContext& Context,
	const TSharedPtr<FRenderPipeline>& Pipeline) {
	if (LineVertices.empty() || !DynamicLineVertexBuffer) {
		return;
	}

	// 정점 버퍼 매핑 및 데이터 복사
	D3D11_MAPPED_SUBRESOURCE MappedVb{};
	HRESULT Result = Context.Map(DynamicLineVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedVb);
	if (FAILED(Result)) {
		LineVertices.clear();
		return;
	}

	const size_t DataSize = sizeof(FVertexData) * LineVertices.size();
	memcpy(MappedVb.pData, LineVertices.data(), DataSize);
	Context.Unmap(DynamicLineVertexBuffer.Get(), 0);

	// 파이프라인 바인딩
	if (Pipeline) {
		Pipeline->Bind(Context);
	}

	// 버퍼 및 토폴로지 바인딩
	const UINT Stride = sizeof(FVertexData);
	const UINT Offset = 0;
	Context.IASetVertexBuffers(0, 1, DynamicLineVertexBuffer.GetAddressOf(), &Stride, &Offset);

	Context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 일괄 드로우콜 호출
	Context.Draw(static_cast<UINT>(LineVertices.size()), 0);

	// 다음 프레임을 위한 버퍼 비우기
	LineVertices.clear();
}

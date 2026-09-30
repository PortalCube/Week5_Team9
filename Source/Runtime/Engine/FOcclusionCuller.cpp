#include "FOcclusionCuller.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FCameraProjection.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UInstancePrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "Runtime/Engine/FSceneTransforms.h"

namespace
{
	// 행벡터 규약 Clip = (P, 1) * M. w로 나누지 않은 클립 좌표 4성분
	FVector4 TransformToClip(const FMatrix& M, const FVector& P)
	{
		return FVector4{
			P.X * M.M[0][0] + P.Y * M.M[1][0] + P.Z * M.M[2][0] + M.M[3][0],
			P.X * M.M[0][1] + P.Y * M.M[1][1] + P.Z * M.M[2][1] + M.M[3][1],
			P.X * M.M[0][2] + P.Y * M.M[1][2] + P.Z * M.M[2][2] + M.M[3][2],
			P.X * M.M[0][3] + P.Y * M.M[1][3] + P.Z * M.M[2][3] + M.M[3][3] };
	}

	// 박스 꼭짓점 번호: bit0 = X(+), bit1 = Y(+), bit2 = Z(+)
	FVector BoxCorner(const FVector& Center, const FVector& Extent, int32 Index)
	{
		return FVector{
			Center.X + ((Index & 1) ? Extent.X : -Extent.X),
			Center.Y + ((Index & 2) ? Extent.Y : -Extent.Y),
			Center.Z + ((Index & 4) ? Extent.Z : -Extent.Z) };
	}

	// 박스 면 6개 × 삼각형 2개 (꼭짓점 번호)
	//더이상 사용하지 않습니다.
	constexpr int32 BoxTriangles[12][3] =
	{
		{ 0, 2, 6 }, { 0, 6, 4 },   // -X
		{ 1, 5, 7 }, { 1, 7, 3 },   // +X
		{ 0, 4, 5 }, { 0, 5, 1 },   // -Y
		{ 2, 3, 7 }, { 2, 7, 6 },   // +Y
		{ 0, 1, 3 }, { 0, 3, 2 },   // -Z
		{ 4, 6, 7 }, { 4, 7, 5 },   // +Z
	};

	// 2D 외적 (= Edge 함수). O→A→B가 반시계로 꺾이면 양수
	float Cross2D(float OX, float OY, float AX, float AY, float BX, float BY)
	{
		return (AX - OX) * (BY - OY) - (AY - OY) * (BX - OX);
	}

	// 점들의 볼록 껍질 (Andrew monotone chain). 결과는 반시계, 최대 Count개
	// 박스가 볼록이므로 "투영된 꼭짓점들의 볼록 껍질 = 박스의 화면 윤곽"
	template <typename TVertex>
	int32 ConvexHull2D(const TVertex* In, int32 Count, TVertex* Out)
	{
		TVertex Points[8];
		for (int32 i = 0; i < Count; ++i) { Points[i] = In[i]; }
		std::sort(Points, Points + Count, [](const TVertex& L, const TVertex& R)
				  {
					  return (L.X < R.X) || (L.X == R.X && L.Y < R.Y);
				  });

		TVertex Hull[16];
		int32 K = 0;
		auto TurnsLeft = [&](const TVertex& P)
			{
				return Cross2D(Hull[K - 2].X, Hull[K - 2].Y, Hull[K - 1].X, Hull[K - 1].Y, P.X, P.Y) > 0.0f;
			};

		for (int32 i = 0; i < Count; ++i)                     // 아래 껍질
		{
			while (K >= 2 && !TurnsLeft(Points[i])) { --K; }
			Hull[K++] = Points[i];
		}
		for (int32 i = Count - 2, Lower = K + 1; i >= 0; --i) // 위 껍질
		{
			while (K >= Lower && !TurnsLeft(Points[i])) { --K; }
			Hull[K++] = Points[i];
		}

		const int32 HullCount = K - 1;                         // 마지막 점은 첫 점과 같음
		for (int32 i = 0; i < HullCount; ++i) { Out[i] = Hull[i]; }
		return HullCount;
	}

	// 삼각형 ABC와 박스(Center ± Half)가 겹치는가. 분리축 정리(SAT), 축 13개
	bool TriangleOverlapsBox(const FVector& Center, const FVector& Half, FVector A, FVector B, FVector C)
	{
		// 박스 중심을 원점으로
		A = A - Center;
		B = B - Center;
		C = C - Center;

		// 이 축에 투영했을 때 삼각형 구간과 박스 구간이 떨어져 있으면 분리됨
		auto IsSeparated = [&](const FVector& Axis) -> bool
			{
				if (Axis.SizeSquared() < 1e-12f) { return false; }   // 퇴화 축은 판단에 쓰지 않음

				const float PA = Axis.Dot(A);
				const float PB = Axis.Dot(B);
				const float PC = Axis.Dot(C);

				const float TriMin = std::min({ PA, PB, PC });
				const float TriMax = std::max({ PA, PB, PC });

				// 박스를 축에 투영한 반구간 = |축|·e  (Frustum의 r = |n|·e와 같은 식)
				const float R = Half.X * std::fabs(Axis.X) + Half.Y * std::fabs(Axis.Y) + Half.Z * std::fabs(Axis.Z);
				return TriMin > R || TriMax < -R;
			};

		const FVector BoxAxes[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
		const FVector Edges[3] = { B - A, C - B, A - C };

		//월드 박스 축
		for (const FVector& Axis : BoxAxes)
		{
			if (IsSeparated(Axis)) 
			{ 
				return false; 
			}
		}

		// 삼각형 법선 1
		if (IsSeparated(Edges[0].Cross(Edges[1])))        
		{
			return false;
		}

		// 박스 축 × 삼각형 변 9
		for (const FVector& BoxAxis : BoxAxes)            
		{
			for (const FVector& Edge : Edges)
			{
				if (IsSeparated(BoxAxis.Cross(Edge))) { return false; }
			}
		}
		return true;   // 어떤 축으로도 분리되지 않음 → 겹침
	}

	// 메시 삼각형 순회 (인덱스 유무 모두 처리)
	template <typename TFunc>
	void ForEachTriangle(const FMesh& Mesh, TFunc&& Func)
	{
		const TArray<FVector>& Positions = Mesh.GetPositions();
		const TArray<uint32>& Indices = Mesh.GetIndices();
		const size_t Count = Mesh.HasIndices() ? Indices.size() : Positions.size();

		//메시 삼각형 돌기
		for (size_t t = 0; t + 2 < Count; t += 3)
		{
			const uint32 I0 = Mesh.HasIndices() ? Indices[t] : static_cast<uint32>(t);
			const uint32 I1 = Mesh.HasIndices() ? Indices[t + 1] : static_cast<uint32>(t + 1);
			const uint32 I2 = Mesh.HasIndices() ? Indices[t + 2] : static_cast<uint32>(t + 2);
			if (I0 >= Positions.size() || I1 >= Positions.size() || I2 >= Positions.size()) { continue; }

			if (Func(Positions[I0], Positions[I1], Positions[I2])) { return; }   // true면 중단
		}
	}

	// 점에서 한 방향으로 쏜 반직선이 표면을 홀수 번 지나면 안쪽 (닫힌 메시 가정)
	bool IsInsideAlongRay(const FMesh& Mesh, const FVector& Point, const FVector& Direction)
	{
		const FRay Ray{ Point, Direction };
		uint32 Hits = 0;
		ForEachTriangle(Mesh, [&](const FVector& A, const FVector& B, const FVector& C)
						{
							float T = 0.0f;
							if (FRayCastingManager::RayIntersectsTriangle(Ray, A, B, C, T)) { ++Hits; }
							return false;
						});
		return (Hits % 2) == 1;
	}

	// 세 방향 모두 안쪽일 때만 인정 (모서리를 스치는 반직선의 오판 대비, 축과 살짝 어긋난 방향 사용)
	bool IsPointInsideMesh(const FMesh& Mesh, const FVector& Point)
	{
		return IsInsideAlongRay(Mesh, Point, FVector{ 1.0f, 0.0013f, 0.0021f })
			&& IsInsideAlongRay(Mesh, Point, FVector{ 0.0017f, 1.0f, 0.0011f })
			&& IsInsideAlongRay(Mesh, Point, FVector{ 0.0019f, 0.0023f, 1.0f });
	}

	FOccluderShape ComputeInnerBox(const FMesh& Mesh)
	{
		FOccluderShape Shape;

		const FAxisAlignedBoundingBox& Local = Mesh.GetLocalBounds();
		if (!Local.IsValid()) { return Shape; }

		// 중심이 메시 안쪽이어야 한다.
		if (!IsPointInsideMesh(Mesh, Local.Center)) { return Shape; }

		// 2) 표면과 닿지 않는 가장 큰 비율 s를 이분 탐색 (박스가 클수록 닿기 쉬우므로 단조)
		auto TouchesSurface = [&](const FVector& Half)
			{
				bool bTouches = false;
				ForEachTriangle(Mesh, [&](const FVector& A, const FVector& B, const FVector& C)
								{
									bTouches = TriangleOverlapsBox(Local.Center, Half, A, B, C);
									return bTouches;   // 하나라도 닿으면 중단
								});
				return bTouches;
			};

		float Lo = 0.0f;
		float Hi = 1.0f;
		for (int32 Iter = 0; Iter < 20; ++Iter)
		{
			const float Mid = (Lo + Hi) * 0.5f;
			if (TouchesSurface(Local.Extent * Mid)) { Hi = Mid; }
			else { Lo = Mid; }
		}

		const float Scale = Lo * 0.98f;   // 부동소수 오차 여유 (안쪽으로)
		if (Scale < 0.05f) { return Shape; }

		Shape.Center = Local.Center;
		Shape.Extent = Local.Extent * Scale;
		Shape.bValid = true;

		UE_LOG("[Occlusion] 내접 박스 계산: 스케일 %.3f (Extent %.3f, %.3f, %.3f)",
			   Scale, Shape.Extent.X, Shape.Extent.Y, Shape.Extent.Z);
		return Shape;
	}

	// 오클루전 대상: 불투명 메시만 (빌보드/텍스트/인스턴싱/스포트라이트 제외)
	bool IsOcclusionTarget(const UPrimitiveComponent& Prim, const FAxisAlignedBoundingBox& WorldBounds)
	{
		if (!Prim.GetMeshAsset() || !Prim.GetMeshAsset()->Get()) { return false; }
		if (Prim.Cast<UBillBoardComp>() || Prim.Cast<UInstancePrimitiveComponent>() || Prim.Cast<USpotLightComponent>())
		{
			return false;
		}
		//항상 보이게 하는 Huge 박스는 제외
		return WorldBounds.Extent.X < 1.0e29f;   // "항상 가시" 표식은 제외
	}
}

void FOcclusionBuffer::Resize(int32 InWidth, int32 InHeight)
{
	Width = std::max(1, InWidth);
	Height = std::max(1, InHeight);
	Depth.resize(static_cast<size_t>(Width) * Height);
}

void FOcclusionBuffer::Clear()
{
	std::fill(Depth.begin(), Depth.end(), (std::numeric_limits<float>::max)());
	CoveredPixels = 0;
	MinWrittenW = (std::numeric_limits<float>::max)();
}

void FOcclusionBuffer::RasterizeBox(const FMatrix& ClipMVP, const FVector& Center, 
									const FVector& Extent)
{
	FScreenVertex V[8];
	if (!ProjectBoxCorners(ClipMVP, Center, Extent, V))
	{
		return;
	}

	FScreenVertex Hull[8];
	const int32 HullCount = ConvexHull2D(V, 8, Hull);
	if (HullCount < 3) return;

	// 박스 위 모든 점의 w ≤ 꼭짓점 w의 최댓값
	float FarW = V[0].W;
	for (int32 i = 1; i < 8; ++i) 
	{ 
		FarW = std::max(FarW, V[i].W); 
	}

	RasterizeConvexPolygon(Hull, HullCount, FarW);

	/*for (const auto& Tri : BoxTriangles)
	{
		RasterizeTriangle(V[Tri[0]], V[Tri[1]], V[Tri[2]]);
	}*/
}

bool FOcclusionBuffer::IsBoxOccluded(const FMatrix& ClipVP, const FMatrix& AbsClipVP, const FVector& Center,
									 const FVector& Extent) const
{
	const FVector4 C = TransformToClip(ClipVP, Center);

	// 각 클립 성분이 박스 안에서 중심으로부터 벗어날 수 있는 최대량 (= |M|·e)
	const float Rx = Extent.X * AbsClipVP.M[0][0] + Extent.Y * AbsClipVP.M[1][0] + Extent.Z * AbsClipVP.M[2][0];
	const float Ry = Extent.X * AbsClipVP.M[0][1] + Extent.Y * AbsClipVP.M[1][1] + Extent.Z * AbsClipVP.M[2][1];
	const float Rw = Extent.X * AbsClipVP.M[0][3] + Extent.Y * AbsClipVP.M[1][3] + Extent.Z * AbsClipVP.M[2][3];

	const float WLo = C.W - Rw;   // 박스에서 가장 가까운 점의 w (정확한 값)
	const float WHi = C.W + Rw;

	if (WLo < NearW) { return false; }          // Near에 걸침 → 판정 불가, 가시
	if (WLo <= MinWrittenW) { return false; }   // D-3: 버퍼의 어떤 기록보다도 앞 → 가려질 수 없음

	// x/w, y/w의 최소·최대는 구간 끝점 조합에서 나온다 (w > 0)
	const float InvLo = 1.0f / WLo;
	const float InvHi = 1.0f / WHi;
	const float XLo = C.X - Rx, XHi = C.X + Rx;
	const float YLo = C.Y - Ry, YHi = C.Y + Ry;

	const float NdcXMin = std::min(XLo * InvLo, XLo * InvHi);
	const float NdcXMax = std::max(XHi * InvLo, XHi * InvHi);
	const float NdcYMin = std::min(YLo * InvLo, YLo * InvHi);
	const float NdcYMax = std::max(YHi * InvLo, YHi * InvHi);

	// NDC → 픽셀 (y는 뒤집힘: NDC 위쪽 = 화면 위쪽 = 작은 픽셀 Y)
	const float MinX = (NdcXMin * 0.5f + 0.5f) * Width;
	const float MaxX = (NdcXMax * 0.5f + 0.5f) * Width;
	const float MinY = (0.5f - NdcYMax * 0.5f) * Height;
	const float MaxY = (0.5f - NdcYMin * 0.5f) * Height;

	// 바깥쪽으로 올림 (조금이라도 걸친 픽셀은 모두 검사)
	const int32 X0 = std::max(0, static_cast<int32>(std::floor(MinX)));
	const int32 X1 = std::min(Width - 1, static_cast<int32>(std::ceil(MaxX)) - 1);
	const int32 Y0 = std::max(0, static_cast<int32>(std::floor(MinY)));
	const int32 Y1 = std::min(Height - 1, static_cast<int32>(std::ceil(MaxY)) - 1);
	if (X0 > X1 || Y0 > Y1) { return false; }

	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		const float* Row = &Depth[static_cast<size_t>(Y) * Width];
		for (int32 X = X0; X <= X1; ++X)
		{
			if (Row[X] >= WLo) { return false; }   // 막히지 않은 픽셀 → 보일 수 있음
		}
	}
	return true;






	//FScreenVertex V[8];
	////하나라도 Near에 걸친다면 중단하고 그리게 한다.
	//if (!ProjectBoxCorners(ClipVP, Center, Extent, V)) 
	//{
	//	return false; 
	//}

	//float MinX = V[0].X;
	//float MaxX = V[0].X;
	//float MinY = V[0].Y;
	//float MaxY = V[0].Y;
	//float NearestW = V[0].W;

	////8개 중에서 가장 작고, 가장 크고, 가장 가까운 뎁스를 찾는다.
	//for (int32 i = 1; i < 8; ++i)
	//{
	//	MinX = std::min(MinX, V[i].X);
	//	MaxX = std::max(MaxX, V[i].X);
	//	MinY = std::min(MinY, V[i].Y);
	//	MaxY = std::max(MaxY, V[i].Y);
	//	NearestW = std::min(NearestW, V[i].W);
	//}

	//// 바깥쪽으로 올림: 박스가 조금이라도 걸친 픽셀은 모두 검사 (화면 밖 부분은 원래 안 보이므로 잘라도 됨)
	//const int32 X0 = std::max(0, static_cast<int32>(std::floor(MinX)));
	//const int32 X1 = std::min(Width - 1, static_cast<int32>(std::ceil(MaxX)) - 1);
	//const int32 Y0 = std::max(0, static_cast<int32>(std::floor(MinY)));
	//const int32 Y1 = std::min(Height - 1, static_cast<int32>(std::ceil(MaxY)) - 1);

	//// 화면에 걸친 픽셀이 없음 → 판단하지 않음
	//if (X0 > X1 || Y0 > Y1) 
	//{ 
	//	return false; 
	//}

	//for (int32 Y = Y0; Y <= Y1; ++Y)
	//{
	//	for (int32 X = X0; X <= X1; ++X)
	//	{
	//		// 저장된 뎁스보다 가까운 뎁스를 갖고 있다면 그려야 한다.
	//		if (Depth[static_cast<size_t>(Y) * Width + X] >= NearestW) 
	//		{ 
	//			return false; 
	//		}
	//	}
	//}

	////여기까지 왔다면 모든 픽셀이 저장된 뎁스보다 멀리 있어서 가려진다.
	//return true;
}

bool FOcclusionBuffer::SaveToBMP(const char* Path) const
{
	FILE* File = nullptr;
	if (fopen_s(&File, Path, "wb") != 0 || !File) { return false; }

	const int32 RowBytes = (Width * 3 + 3) & ~3;   // BMP 행은 4바이트 정렬
	const uint32 ImageBytes = static_cast<uint32>(RowBytes * Height);
	uint8 Header[54] = { 'B', 'M' };
	auto Put32 = [&](int32 Offset, uint32 Value) { memcpy(&Header[Offset], &Value, 4); };
	Put32(2, 54 + ImageBytes); Put32(10, 54); Put32(14, 40);
	Put32(18, static_cast<uint32>(Width)); Put32(22, static_cast<uint32>(Height));
	Header[26] = 1; Header[28] = 24;
	Put32(34, ImageBytes);
	fwrite(Header, 1, 54, File);

	float MaxW = 1.0f;
	for (float W : Depth) { if (W < (std::numeric_limits<float>::max)()) { MaxW = std::max(MaxW, W); } }

	TArray<uint8> Row(RowBytes, 0);
	for (int32 Y = Height - 1; Y >= 0; --Y)   // BMP는 아래 행부터 저장
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const float W = Depth[static_cast<size_t>(Y) * Width + X];
			const uint8 Gray = (W >= (std::numeric_limits<float>::max)())
				? 0 : static_cast<uint8>(255.0f - 200.0f * (W / MaxW));
			Row[X * 3 + 0] = Row[X * 3 + 1] = Row[X * 3 + 2] = Gray;
		}
		fwrite(Row.data(), 1, RowBytes, File);
	}
	fclose(File);
	return true;
}

bool FOcclusionBuffer::ProjectBoxCorners(const FMatrix& Clip, const FVector& Center, 
										 const FVector& Extent, FScreenVertex Out[8]) const
{
	for (int32 i = 0; i < 8; i++)
	{
		const FVector4 C = TransformToClip(Clip, BoxCorner(Center, Extent, i));
		//하나라도 Near보다 작다면 그리게 한다.
		if (C.W < NearW)
		{
			return false;
		}

		const float InvW = 1.f / C.W;
		Out[i].X = (C.X * InvW * 0.5f + 0.5f) * Width;    // NDC x -1~1 → 0~Width
		Out[i].Y = (0.5f - C.Y * InvW * 0.5f) * Height;   // NDC y는 위가 +1, 화면은 아래가 +
		Out[i].W = C.W;
	}
	return true;
}

//더이상 사용하지 않습니다.
void FOcclusionBuffer::RasterizeTriangle(const FScreenVertex& V0, const FScreenVertex& V1, 
										 const FScreenVertex& V2)
{
	//Edge : 점 P가 변 A-B의 어느쪽에 있는가(부호있는 넓이 * 2)
	auto Edge = [](const FScreenVertex& A, const FScreenVertex& B, float PX, float PY)
		{
			return (B.X - A.X) * (PY - A.Y) - (B.Y - A.Y) * (PX - A.X);
		};

	//감김 방향을 통일 : 넓이가 양수가 되도록 두 꼭짓점을 바꾼다.
	const float Area = Edge(V0, V1, V2.X, V2.Y);

	if (std::fabs(Area) < 1e-8f) { return; }   // 화면에서 선분/점: 덮는 픽셀 없음
	const FScreenVertex& A = V0;
	const FScreenVertex& B = (Area > 0.0f) ? V1 : V2;
	const FScreenVertex& C = (Area > 0.0f) ? V2 : V1;

	auto IsInside = [&](float PX, float PY)
		{
			return Edge(A, B, PX, PY) >= 0.0f && Edge(B, C, PX, PY) >= 0.0f && Edge(C, A, PX, PY) >= 0.0f;
		};

	// 가장 먼 거리
	const float FarW = std::max({ A.W, B.W, C.W });

	//Min은 floor로, Max는 ceil로 바깥쪽으로 올린다.
	const int32 MinX = std::max(0, static_cast<int32>(std::floor(std::min({ A.X, B.X, C.X }))));
	const int32 MaxX = std::min(Width - 1, static_cast<int32>(std::ceil(std::max({ A.X, B.X, C.X }))));
	const int32 MinY = std::max(0, static_cast<int32>(std::floor(std::min({ A.Y, B.Y, C.Y }))));
	const int32 MaxY = std::min(Height - 1, static_cast<int32>(std::ceil(std::max({ A.Y, B.Y, C.Y }))));

	//픽셀 돌면서 뎁스 채우기
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			// 픽셀 [X, X+1] × [Y, Y+1]의 네 모서리가 모두 안 → 픽셀 전체가 덮임 (삼각형은 볼록)
			const float X0 = static_cast<float>(X);
			const float X1 = X0 + 1.0f;
			const float Y0 = static_cast<float>(Y);
			const float Y1 = Y0 + 1.0f;
			if (!IsInside(X0, Y0) || !IsInside(X1, Y0) || !IsInside(X0, Y1) || !IsInside(X1, Y1)) { continue; }

			//해당 픽셀 위치의 뎁스 값
			float& Pixel = Depth[static_cast<size_t>(Y) * Width + X];

			//이미 존재하는 뎁스 값이 있다면 비교하여 더 가까운 거로
			Pixel = std::min(Pixel, FarW);
		}
	}
}

void FOcclusionBuffer::RasterizeConvexPolygon(const FScreenVertex* Polygon, int32 Count, float FarW)
{
	constexpr float Infinity = (std::numeric_limits<float>::max)();
	constexpr float SpanEpsilon = 1e-3f;   // 부동소수 오차 대비. 구간을 안쪽으로 줄인다 (보수적)

	float MinXf = Polygon[0].X;
	float MaxXf = Polygon[0].X;
	float MinYf = Polygon[0].Y;
	float MaxYf = Polygon[0].Y;

	//4 모서리 Min, Max 갱신
	for (int32 i = 1; i < Count; ++i)
	{
		MinXf = std::min(MinXf, Polygon[i].X);
		MaxXf = std::max(MaxXf, Polygon[i].X);
		MinYf = std::min(MinYf, Polygon[i].Y);
		MaxYf = std::max(MaxYf, Polygon[i].Y);
	}

	// 가로나 세로가 1픽셀 미만이면 완전히 덮는 픽셀이 있을 수 없다
	if (MaxXf - MinXf < 1.0f || MaxYf - MinYf < 1.0f) 
	{ 
		return; 
	}

	// 가로선 y에서 다각형 내부 구간 [OutL, OutR]. 가로선이 다각형을 지나지 않으면 false
	auto SpanAt = [&](float Y, float& OutL, float& OutR) -> bool
		{
			OutL = Infinity;
			OutR = -Infinity;
			for (int32 i = 0; i < Count; ++i)
			{
				const FScreenVertex& A = Polygon[i];
				const FScreenVertex& B = Polygon[(i + 1) % Count];

				// 수평 변: 끝점은 이웃 변이 처리
				if (A.Y == B.Y)  continue; 
				if (Y < std::min(A.Y, B.Y) || Y > std::max(A.Y, B.Y))  continue; 

				const float X = A.X + (Y - A.Y) * (B.X - A.X) / (B.Y - A.Y);
				OutL = std::min(OutL, X);
				OutR = std::max(OutR, X);
			}
			return OutL <= OutR;
		};

	// 행 Y는 가로선 Y ~ Y+1 사이의 띠. 윗선 Y ≥ MinYf, 아랫선 Y+1 ≤ MaxYf 인 행만 완전히 덮일 수 있다
	const int32 FirstRow = std::max(0, static_cast<int32>(std::ceil(MinYf)));
	const int32 EndRow = std::min(Height, static_cast<int32>(std::floor(MaxYf)));   // [FirstRow, EndRow)

	float TopL = 0.0f;
	float TopR = 0.0f;
	bool bTop = SpanAt(static_cast<float>(FirstRow), TopL, TopR);

	for (int32 Y = FirstRow; Y < EndRow; ++Y)
	{
		float BottomL = 0.0f, BottomR = 0.0f;
		const bool bBottom = SpanAt(static_cast<float>(Y + 1), BottomL, BottomR);   // 아랫선은 다음 행의 윗선

		if (bTop && bBottom)
		{
			// 네 모서리가 모두 안쪽 = 두 가로선 구간의 교집합 안에 [X, X+1]이 들어감
			const float L = std::max(TopL, BottomL) + SpanEpsilon;
			const float R = std::min(TopR, BottomR) - SpanEpsilon;
			const int32 FirstX = std::max(0, static_cast<int32>(std::ceil(L)));
			const int32 EndX = std::min(Width, static_cast<int32>(std::floor(R)));   // [FirstX, EndX), X+1 ≤ R

			float* Row = &Depth[static_cast<size_t>(Y) * Width];
			for (int32 X = FirstX; X < EndX; ++X)
			{
				if (FarW < Row[X])
				{
					CoveredPixels += (Row[X] == Infinity) ? 1 : 0;
					Row[X] = FarW;
				}
			}
			if (FirstX < EndX) { MinWrittenW = std::min(MinWrittenW, FarW); }
		}

		TopL = BottomL; 
		TopR = BottomR; 
		bTop = bBottom;
	}






	//// 픽셀 모서리 점이 다각형 안에 있는가: 모든 변에 대해 Edge ≥ 0 (반시계라 안쪽이 양수)
	//auto IsInside = [&](float PX, float PY)
	//	{
	//		for (int32 i = 0; i < Count; ++i)
	//		{
	//			const FScreenVertex& A = Polygon[i];
	//			const FScreenVertex& B = Polygon[(i + 1) % Count];
	//			if (Cross2D(A.X, A.Y, B.X, B.Y, PX, PY) < 0.0f) { return false; }
	//		}
	//		return true;
	//	};

	//float MinXf = Polygon[0].X;
	//float MaxXf = Polygon[0].X;
	//float MinYf = Polygon[0].Y;
	//float MaxYf = Polygon[0].Y;

	//for (int32 i = 1; i < Count; ++i)
	//{
	//	MinXf = std::min(MinXf, Polygon[i].X); 
	//	MaxXf = std::max(MaxXf, Polygon[i].X);
	//	MinYf = std::min(MinYf, Polygon[i].Y); 
	//	MaxYf = std::max(MaxYf, Polygon[i].Y);
	//}

	//
	////Min은 floor, Max는 ceil
	//const int32 MinX = std::max(0, static_cast<int32>(std::floor(MinXf)));
	//const int32 MaxX = std::min(Width - 1, static_cast<int32>(std::ceil(MaxXf)));
	//const int32 MinY = std::max(0, static_cast<int32>(std::floor(MinYf)));
	//const int32 MaxY = std::min(Height - 1, static_cast<int32>(std::ceil(MaxYf)));

	////픽셀마다 안쪽인지 판정
	//for (int32 Y = MinY; Y <= MaxY; ++Y)
	//{
	//	for (int32 X = MinX; X <= MaxX; ++X)
	//	{
	//		// 픽셀 네 모서리가 모두 안 → 픽셀 전체가 덮임 (다각형은 볼록)
	//		const float X0 = static_cast<float>(X), X1 = X0 + 1.0f;
	//		const float Y0 = static_cast<float>(Y), Y1 = Y0 + 1.0f;
	//		if (!IsInside(X0, Y0) || !IsInside(X1, Y0) || !IsInside(X0, Y1) || !IsInside(X1, Y1)) { continue; }

	//		float& Pixel = Depth[static_cast<size_t>(Y) * Width + X];
	//		Pixel = std::min(Pixel, FarW);
	//	}
	//}
}

uint32 FOcclusionCuller::Cull(const FSceneView& View, const UScene& Scene, 
							  TArray<uint8>& InOutVisibleFlags, TArray<uint8>& OutOccludedFlags)
{
	const TArray<UPrimitiveComponent*>& Prims = Scene.GetRenderComponents();
	const TArray<FAxisAlignedBoundingBox>& Bounds = Scene.GetCullDataList();
	OutOccludedFlags.assign(Prims.size(), 0);

	// 월드 → D3D 클립
	FMatrix ClipVP = View.ViewProj;
	ClipVP = ClipVP.ToD3DMatrix();
	const FMatrix AbsClipVP = ClipVP.Abs();

	const float Aspect = View.Camera.GetProjection().GetAspectRatio() > 0.0f ? View.Camera.GetProjection().GetAspectRatio() : 1.0f;
	
	//오클루전 버퍼 초기화
	Buffer.Resize(BufferWidth, static_cast<int32>(BufferWidth / Aspect));
	Buffer.SetNearW(View.Camera.GetProjection().GetNearPlane());
	Buffer.Clear();

	// 후보 수집, 가까운 K개를 앞으로
	// TODO : 가까운 순서가 아닌 화면에 대한 영향이 큰 점수대로 수정.
	// 내접 박스 크기 / 카메라 거리 제곱으로 점수
	// 내접 박스 부피 / 로컬 aabb 부피로 점수(크기만 크고 내접 박스가 작은 의자와 같은 것을 거른다)
	size_t OccluderCount = 0;
	{
		SCOPE_CYCLE_COUNTER("OcclusionSelect");
		Candidates.clear();
		Candidates.reserve(Prims.size());

		//Prims[]로 접근하는 것보다 연속된 배열인 TargetFlags로 가져오는게 캐시 효율이 좋다.
		const TArray<uint8>& TargetFlags = Scene.GetOcclusionTargetFlags();

		// 중심의 w만 필요하다 → 클립 좌표 4번째 성분 = (x, y, z, 1) · (4번째 열)
		// 전부 행렬 계산할 거까진 없다.
		const FVector4 VP_W = FVector4{ ClipVP.M[0][3], ClipVP.M[1][3], ClipVP.M[2][3], ClipVP.M[3][3] };

		//오클루전 컬링 후보자들 수집
		for (size_t i = 0; i < Prims.size(); ++i)
		{
			// Frustum 통과 여부와 대상 여부
			//기존인 if (!InOutVisibleFlags[i] || !Prims[i]) 보다 4 ~ 5배 빠름
			//기존 3.6ms -> TargetFlags[i] 0.87ms
			if (!InOutVisibleFlags[i] || !TargetFlags[i]) { continue; }

			//w를 얻기 위해 vp 전체 행렬 곱을 하지 않고 3열 값만 가져와서 계산한다.
			//const float CenterW = TransformToClip(ClipVP, Bounds[i].Center).W;
			//Candidates.push_back({ static_cast<uint32>(i), CenterW });

			const FVector& Center = Bounds[i].Center;
			Candidates.push_back({ static_cast<uint32>(i), 
								 Center.X * VP_W.X + Center.Y * VP_W.Y + Center.Z * VP_W.Z + VP_W.W });
		}

		OccluderCount = std::min<size_t>(OccluderBudget, Candidates.size());
		if (OccluderCount < Candidates.size())
		{
			// 전체 정렬 대신 "가까운 K개만 앞쪽에" (O(n))
			std::nth_element(Candidates.begin(), Candidates.begin() + OccluderCount, Candidates.end(),
							 [](const FCandidate& L, const FCandidate& R) { return L.Depth < R.Depth; });
		}
	}

	// Occluder 래스터화
	{
		SCOPE_CYCLE_COUNTER("OcclusionRaster");

		const FSceneTransforms& Transforms = Scene.GetSceneTransforms();

		for (size_t k = 0; k < OccluderCount; ++k)
		{
			const uint32 Index = Candidates[k].SceneIndex;
			const UPrimitiveComponent* Prim = Prims[Index];
			const FOccluderShape& Shape = GetOccluderShape(*Prim->GetMeshAsset()->Get());
			
			//내접 박스를 그릴 수 없었다. 통과.
			if (!Shape.bValid) { continue; }

			// 월드 행렬은 UpdateDirtyBounds에서 이미 최신 (BatchIndex == SceneIndex)
			// GetGlobalTransform() 재계산이 필요 없다
			const FMatrix& World = Transforms.WorldMatrices[Index];
			Buffer.RasterizeBox(World * ClipVP, Shape.Center, Shape.Extent);
		}



		////Occluder로 사용할 오브젝트들을 래스터화
		//for (size_t k = 0; k < OccluderCount; ++k)
		//{
		//	const UPrimitiveComponent* Prim = Prims[Candidates[k].SceneIndex];
		//	const FOccluderShape& Shape = GetOccluderShape(*Prim->GetMeshAsset()->Get());

		//	if (!Shape.bValid) { continue; }

		//	// 회전을 포함한 실제 박스(OBB)를 그린다. 월드 AABB로 감싸면 메시 밖으로 나가므로 안 됨
		//	const FMatrix World = Prim->GetGlobalTransform().GetMatrix();
		//	Buffer.RasterizeBox(World * ClipVP, Shape.Center, Shape.Extent);
		//}
	}

	if (bDumpNextFrame)
	{
		Buffer.SaveToBMP("OcclusionBuffer.bmp");
		bDumpNextFrame = false;
		UE_LOG("[Occlusion] OcclusionBuffer.bmp 저장 (%d x %d)", Buffer.GetWidth(), Buffer.GetHeight());
	}

	// 덮인 픽셀이 너무 적으면 이번 프레임은 판정을 생략 (지울 수 있는 게 거의 없다)
	const int32 MinCoveredPixels = static_cast<int32>(Buffer.GetWidth() * Buffer.GetHeight() * MinCoveredRatio);
	if (Buffer.GetCoveredPixels() < MinCoveredPixels)
	{
		INC_DWORD_STAT("OcclusionSkipped");
		INC_DWORD_STAT_BY("Occluders", OccluderCount);
		return 0;
	}

	// Occludee 판정 (Occluder 자신은 가장 앞쪽이라 판정하지 않고 그린다)
	// Occluder들은 후보자 배열 앞쪽에 모여있으므로 건너뛰는 것만으로 그릴 대상에 포함된다.
	uint32 OccludedCount = 0;
	{
		SCOPE_CYCLE_COUNTER("OcclusionTest");
		const size_t StartIndex = bIncludeOccluderCull ? 0 : OccluderCount;
		for (size_t k = StartIndex; k < Candidates.size(); ++k)
		{
			const uint32 Index = Candidates[k].SceneIndex;
			const FAxisAlignedBoundingBox& Box = Bounds[Index];

			if (Buffer.IsBoxOccluded(ClipVP, AbsClipVP, Box.Center, Box.Extent))
			{
				InOutVisibleFlags[Index] = 0;
				OutOccludedFlags[Index] = 1;
				++OccludedCount;
			}
		}



		//SCOPE_CYCLE_COUNTER("OcclusionTest");
		//size_t StartIndex = bIncludeOccluderCull ? 0 : OccluderCount;
		//for (size_t k = StartIndex; k < Candidates.size(); ++k)
		//{
		//	const uint32 Index = Candidates[k].SceneIndex;
		//	const FAxisAlignedBoundingBox& Box = Bounds[Index];

		//	//후보자들의 AABB로 체크한다. Occludee할 때는 AABB 박스로 체크하여 큰 박스로 보수적으로 확인한다.
		//	if (Buffer.IsBoxOccluded(ClipVP, Box.Center, Box.Extent))
		//	{
		//		InOutVisibleFlags[Index] = 0;
		//		OutOccludedFlags[Index] = 1;
		//		++OccludedCount;
		//	}
		//}
	}

	INC_DWORD_STAT_BY("Occluders", OccluderCount);
	INC_DWORD_STAT_BY("Occluded", OccludedCount);
	return OccludedCount;
}


const FOccluderShape& FOcclusionCuller::GetOccluderShape(const FMesh& Mesh)
{
	auto It = ShapeCache.find(&Mesh);
	if (It != ShapeCache.end()) 
	{ 
		return It->second; 
	}

	return ShapeCache.emplace(&Mesh, ComputeInnerBox(Mesh)).first->second;
}

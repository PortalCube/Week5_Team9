#pragma once
#include "Core.h"

class FMesh;
class UScene;
struct FSceneView;

//메시에 내접한 박스. 메시마다 갖고 있어야 한다.
struct FOccluderShape
{
	FVector Center{};
	FVector Extent{};
	bool bValid = false;
};

//CPU 깊이 버퍼. 픽셀마다 깊이 값을 저장한다.
class FOcclusionBuffer
{
public:
	//Width, Height, Depth 배열 크기 초기화
	void Resize(int32 InWidth, int32 InHeight);
	void SetNearW(float InNearW) { NearW = InNearW; }
	//Depth배열 값 초기화. 막힌 게 없다.
	void Clear();

	//로컬 박스를 삼각형 12개로 그린다.(Occluder : 실제보다 작고 멀게)
	void RasterizeBox(const FMatrix& ClipMVP, const FVector& Center, const FVector& Extent);

	//월드 AABB가 완전히 가려졌는가(Occludee : 실제보다 크고 가깝게)
	bool IsBoxOccluded(const FMatrix& ClipVP, const FVector& Center, const FVector& Extent)const;

	int32 GetWidth() const { return Width; }
	int32 GetHeight() const { return Height; }

	//이미지로 저장
	bool SaveToBMP(const char* Path) const;

private:
	//픽셀 좌표 + 선형 깊이
	struct FScreenVertex
	{
		float X;
		float Y;
		float W;
	};

	//꼭지점 8개를 픽셀 좌표로. 하나라도 Near 앞이면 false(투영 불가)
	bool ProjectBoxCorners(const FMatrix& Clip, const FVector& Center, const FVector& Extent, FScreenVertex Out[8])const;

	//삼각형으로 뎁스 채우기
	void RasterizeTriangle(const FScreenVertex& V0, const FScreenVertex& V1, const FScreenVertex& V2);

	float NearW = 0.1f;
	int32 Width = 0;
	int32 Height = 0;
	TArray<float> Depth;
};

class FOcclusionCuller
{
public:
	//Frustum을 통과한 것중 가려질 것을 0으로 바꾼다.
	//OutOccludedFlags[i] == 1 : 오클루전으로 지운 것. 나중에 오라클 검증.
	//반환값 : 지운 개수
	uint32 Cull(const FSceneView& View, const UScene& Scene, TArray<uint8>& InOutVisibleFlags,
				TArray<uint8>& OutOccludedFlags);

	//Occluder에 사용할 오브젝트 수
	uint32 OccluderBudget = 1024;
	int32 BufferWidth = 512;
	bool bIncludeOccluderCull = false;

	bool bDumpNextFrame = false;

private:
	struct FCandidate
	{
		uint32 SceneIndex;
		//월드 AABB 중심의 w(카메라 앞 방향 거리)
		float Depth;
	};

	const FOccluderShape& GetOccluderShape(const FMesh& Mesh);

	FOcclusionBuffer Buffer;
	TArray<FCandidate> Candidates;
	TMap<const FMesh*, FOccluderShape> ShapeCache;
};
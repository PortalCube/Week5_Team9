#include "FOcclusionCuller.h"

void FOcclusionBuffer::Resize(int32 InWidth, int32 InHeight)
{
}

void FOcclusionBuffer::Clear()
{
}

void FOcclusionBuffer::RasterizeBox(const FMatrix& ClipMVP, const FVector& Center, const FVector& Extent)
{
}

bool FOcclusionBuffer::IsBoxOccluded(const FMatrix& ClipVP, const FVector& Center, const FVector& Extent) const
{
	return false;
}

bool FOcclusionBuffer::ProjectBoxCorners(const FMatrix& Clip, const FVector& Center, const FVector& Extent, FScreenVertex Out[8]) const
{
	return false;
}

void FOcclusionBuffer::RasterizeTriangle(const FScreenVertex& V0, const FScreenVertex& V1, const FScreenVertex& V2)
{
}

uint32 FOcclusionCuller::Cull(const FSceneView& View, const UScene& Scene, TArray<uint8>& InOutVisibleFlags, TArray<uint8>& OutOccludedFlags)
{
	return uint32();
}

const FOccluderShape& FOcclusionCuller::GetOccluderShape(const FMesh& Mesh)
{
	// TODO: 여기에 return 문을 삽입합니다.
	return FOccluderShape();
}

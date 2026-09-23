#pragma once
#include "Source/Runtime/Math/FVector.h"
#include "Source/Runtime/Math/FVector2.h"
#include "Source/Runtime/Math/FVector4.h"
#include "Source/Runtime/Core/TArray.h"
#include "Source/Runtime/Core/TMap.h"
#include "Source/Runtime/Core/FString.h"
#include "Source/Runtime/Core/IntTypes.h"
#include "Source/Runtime/Rendering/Vertices.h"
#include "Source/Runtime/Rendering/FMesh.h"

#include <fstream>
#include <sstream>

struct FObjIndex
{
	int v = 0;
	int vt = 0;
	int vn = 0;

	bool operator==(const FObjIndex& Rhs) const
	{
		return v == Rhs.v && vt == Rhs.vt && vn == Rhs.vn;
	}
};

struct FRawObjData
{
	TArray<FVector> Positions; // v
	TArray<FVector2> TexCoords; // vt
	TArray<FVector> Normals; // vn
	TArray<TArray<FObjIndex>> Faces; // f
	TArray<FMeshSection> Sections; // Mesh Section
};

struct FMtlData
{
	FString MaterialName;
	float Ns; // Specular Power
	float Ni; // Optical Density
	float d; // Transparency
	float Tr; // Transparency
	FVector Tf; // Transmission Filter
	uint8 illum; // Illumination Model
	FVector Ka; // Ambient Color
	FVector Kd; // Diffuse Color
	FVector Ks; // Specular Color
	FVector Ke; // Emissive Color
	FString map_Ka; // Ambient Color Map
	FString map_Kd; // Diffuse Color Map
	FString map_Ks; // Specular Color Map
	FString map_bump; // Bump Map
};

#pragma pack(push, 1)
struct FMeshFileHeader
{
	uint32 Magic = 0x4D455348; // Magin number : 'MESH'
	uint64 SourceHash = 0; // Compare with Source obj hash

	uint32 VertexCount = 0;
	uint32 IndexCount = 0;
	uint32 SectionCount = 0;
};
#pragma pack(pop)

class FObjParser
{
public:
	static bool LoadObj(const char* InFilePath, FRawObjData& OutResult);
	static bool ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections);
	static bool SaveMeshToBinary(const char* OutFilePath, uint64 InSourceHash, const TArray<FVertexData>& InVertices, TArray<uint32>& InIndices, TArray<FMeshSection>& InSections);
	static bool LoadMeshFromBinary(const char* InFilePath, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections);

	static bool LoadMtl(const char* InFilePath, TArray<FMtlData>& OutResult);
	
	// Validate bin file by Magic number and hash
	static bool ValidateBinary(const char* InBinFilePath, const char* InObjFilePath);

	static FObjIndex ParseFaceToken(const FString& Token);
	// FNV-1a hash func
	static uint64 ComputeFileHash(const std::filesystem::path& FilePath);
};
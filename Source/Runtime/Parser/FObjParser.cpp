#include "FObjParser.h"
#include <filesystem>

//////////////////////////////////////////////////////////////////////////
// Helper
inline const char* SkipSpaces(const char* p)
{
    while (*p == ' ' || *p == '\t')
    {
        ++p;
    }
    return p;
}

inline const char* SkipLine(const char* p)
{
    while (*p && *p != '\n' && *p != '\r')
    {
        ++p;
    }
    while (*p == '\n' || *p == '\r')
    {
        ++p;
    }
    return p;
}

// FNV-1a
struct FObjIndexHash
{
    size_t operator()(const FObjIndex& ObjIndex) const
    {
        size_t Hash = 14695981039346656037ULL;
        Hash = (Hash ^ static_cast<size_t>(ObjIndex.v)) * 1099511628211ULL;
        Hash = (Hash ^ static_cast<size_t>(ObjIndex.vn)) * 1099511628211ULL;
        Hash = (Hash ^ static_cast<size_t>(ObjIndex.vt)) * 1099511628211ULL;
        return Hash;
    }
};

//////////////////////////////////////////////////////////////////////////

bool FObjParser::LoadObj(const char* InFilePath, FRawObjData& OutResult)
{
    std::ifstream File(InFilePath, std::ios::binary | std::ios::ate);
    if (!File.is_open())
    {
        return false;
    }

    std::streamsize FileSize = File.tellg();
    File.seekg(0, std::ios::beg);

    TArray<char> Buffer(FileSize + 1);
    if (!File.read(Buffer.data(), FileSize))
    {
        return false;
    }
    Buffer[FileSize] = '\0'; // null

    // file pointer
    const char* p = Buffer.data();
    char* next = nullptr;

    // Mesh Section
    FString CurrentMaterialName = "";
    uint32 CurrentStartindex = 0;  

    
    while (*p)
    {
        p = SkipSpaces(p);
        if (*p == '#' || *p == '\n' || *p == '\r')
        {
            p = SkipLine(p);
            continue;
        }
        
        if (p[0] == 'v' && (p[1] == ' ' || p[1] == '\t')) // Position
        {
            p += 1;

            float x = strtof(p, &next); p = next;
            float y = strtof(p, &next); p = next;
            float z = strtof(p, &next); p = next;

            OutResult.Positions.push_back({ -z ,x , y }); // Change Unreal Coord
        }
        else if (p[0] == 'v' && p[1] == 't' && (p[2] == ' ' || p[2] == '\t')) // Texture Coords
        {
            p += 2;

            float u = strtof(p, &next); p = next;
            float v = strtof(p, &next); p = next;
                      
            OutResult.TexCoords.push_back({u, 1.0f - v}); // Change Unreal Coord
        }
        else if (p[0] == 'v' && p[1] == 'n' && (p[2] == ' ' || p[2] == '\t')) // Normal
        {
            p += 2;

            float x = strtof(p, &next); p = next;
            float y = strtof(p, &next); p = next;
            float z = strtof(p, &next); p = next;
            
            OutResult.Normals.push_back({ -z , x , y}); // Change Unreal Coord
        }
        else if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t')) // Faces
        {
            p += 1;

            FObjIndex FaceIndices[8]; // for all polygon
            int FaceIdx = 0;

            while (*p != '\r' && *p != '\n' && *p != '\0' && FaceIdx < 8)
            {
                p = SkipSpaces(p);

                if (*p == '\r' || *p == '\n' || *p == '\0')
                {
                    break;
                }

                // Face parsing
                FObjIndex ObjIdx{};
                ObjIdx.v = static_cast<int>(strtol(p, &next, 10));
                p = next;
                if (*p == '/')
                {
                    ++p;
                    if (*p != '/')
                    {
                        ObjIdx.vt = static_cast<int>(strtol(p, &next, 10));
                        p = next;
                    }
                    if (*p == '/')
                    {
                        ++p;
                        ObjIdx.vn = static_cast<int>(strtol(p, &next, 10));
                        p = next;
                    }
                    FaceIndices[FaceIdx++] = ObjIdx;
                }
            }

            for (size_t i = 1; i + 1 < FaceIdx; i++)
            {
                OutResult.Faces.push_back({ FaceIndices[0], FaceIndices[i + 1], FaceIndices[i] });  // Change Unreal Coord
            }
        }
        else if (strncmp(p, "usemtl", 6) == 0) // Mesh section
        {
            p += 6;
            p = SkipSpaces(p);

            const char* start = p; 
            while (*p && *p != ' ' && *p !='\t' && *p !='\r' && *p != '\n')
            {
                ++p;
            }

            FString NewMaterialName(start, p - start);

            uint32 NewStartIndex = static_cast<uint32>(OutResult.Faces.size() * 3);
            uint32 NewIndexCount = NewStartIndex - CurrentStartindex;

            if (NewIndexCount > 0)
            {
                FMeshSection NewMeshSection;
                NewMeshSection.SectionName = CurrentMaterialName;
                NewMeshSection.StartIndex = CurrentStartindex;
                NewMeshSection.IndexCount = NewIndexCount;

                OutResult.Sections.push_back(NewMeshSection);

                CurrentStartindex = NewStartIndex;
            }

            CurrentMaterialName = NewMaterialName;
        }

        p = SkipLine(p);
    }

    //FString Line;
    //while (std::getline(File, Line))
    //{
    //    if (Line.empty() || Line[0] == '#')
    //    {
    //        continue;
    //    }

    //    std::stringstream ss(Line);
    //    FString Prefix;
    //    ss >> Prefix;

    //    if (Prefix == "v") // Position
    //    {
    //        FVector Pos;
    //        float x, y, z;
    //        ss >> x >> y >> z;
    //        Pos.X = -z; Pos.Y = x; Pos.Z = y; // Change Unreal Coord
    //        OutResult.Positions.push_back(Pos);
    //    }
    //    else if (Prefix == "vt") // Texture Coords
    //    {
    //        FVector2 Tex;
    //        float u, v;
    //        ss >> u >> v;
    //        Tex.X = u; Tex.Y = 1.0f - v; // Change Unreal Coord
    //        OutResult.TexCoords.push_back(Tex);
    //    }
    //    else if (Prefix == "vn") // Normal
    //    {
    //        FVector Norm;
    //        float x, y, z;
    //        ss >> x >> y >> z;
    //        Norm.X = -z; Norm.Y = x; Norm.Z = y; // Change Unreal Coord
    //        OutResult.Normals.push_back(Norm);
    //    }
    //    else if (Prefix == "f") // Faces
    //    {
    //        TArray<FString> Tokens;
    //        FString Word;
    //        while (ss >> Word)
    //        {
    //            Tokens.push_back(Word);
    //        }

    //        if (Tokens.size() < 3) continue;

    //        TArray<FObjIndex> FaceIndices;
    //        for (const FString& T : Tokens)
    //        {
    //            FaceIndices.push_back(ParseFaceToken(T));
    //        }

    //        for (size_t i = 1; i + 1 < FaceIndices.size(); i++)
    //        {
    //            OutResult.Faces.push_back({ FaceIndices[0], FaceIndices[i + 1], FaceIndices[i] });  // Change Unreal Coord
    //        }
    //    }
    //    else if (Prefix == "usemtl") // Mesh section
    //    {
    //        FString NewMaterialName;
    //        ss >> NewMaterialName;

    //        uint32 NewStartIndex = static_cast<uint32>(OutResult.Faces.size() * 3);
    //        uint32 NewIndexCount = NewStartIndex - CurrentStartindex;

    //        if (NewIndexCount > 0)
    //        {
    //            FMeshSection NewMeshSection;
    //            NewMeshSection.SectionName = CurrentMaterialName;
    //            NewMeshSection.StartIndex = CurrentStartindex;
    //            NewMeshSection.IndexCount = NewIndexCount;

    //            OutResult.Sections.push_back(NewMeshSection);

    //            CurrentStartindex = NewStartIndex;
    //        }

    //        CurrentMaterialName = NewMaterialName;            
    //    }
    //}

    // Final mesh section
    uint32 FinalStartIndex = static_cast<uint32>(OutResult.Faces.size() * 3);
    uint32 FinalndexCount = FinalStartIndex - CurrentStartindex;

    if (FinalndexCount > 0)
    {
        FMeshSection FinalMeshSection;
        FinalMeshSection.SectionName = CurrentMaterialName;
        FinalMeshSection.StartIndex = CurrentStartindex;
        FinalMeshSection.IndexCount = FinalndexCount;

        OutResult.Sections.push_back(FinalMeshSection);
    }

    return true;
}

bool FObjParser::ConvertObjToVertex(const FRawObjData& InObjData, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections)
{
    std::unordered_map<FObjIndex, uint32, FObjIndexHash> UniqueVertex;

    for (const auto& Face : InObjData.Faces)
    {
        for (const auto& Corner : Face)
        {
            // For changing Negative Index and 1-based Index
            int vIdx = (Corner.v < 0) ? (static_cast<int>(InObjData.Positions.size()) + Corner.v) : (Corner.v - 1);
            int vtIdx = (Corner.vt < 0) ? (static_cast<int>(InObjData.TexCoords.size()) + Corner.vt) : (Corner.vt - 1);
            int vnIdx = (Corner.vn < 0) ? (static_cast<int>(InObjData.Normals.size()) + Corner.vn) : (Corner.vn - 1);

            FObjIndex Key{ vIdx, vtIdx, vnIdx };

            auto It = UniqueVertex.find(Key);
            if (It != UniqueVertex.end())
            {
                OutIndices.push_back(It->second);
            }
            else
            {
                FVertexData Vertex{};

                if (vIdx >= 0 && vIdx < static_cast<int>(InObjData.Positions.size()))
                {
                    Vertex.x = InObjData.Positions[vIdx].X;
                    Vertex.y = InObjData.Positions[vIdx].Y;
                    Vertex.z = InObjData.Positions[vIdx].Z;
                }

                if (vtIdx >= 0 && vtIdx < static_cast<int>(InObjData.TexCoords.size()))
                {
                    Vertex.u = InObjData.TexCoords[vtIdx].X;
                    Vertex.v = InObjData.TexCoords[vtIdx].Y;
                }

                if (vnIdx >= 0 && vnIdx < static_cast<int>(InObjData.Normals.size()))
                {
                    Vertex.nx = InObjData.Normals[vnIdx].X;
                    Vertex.ny = InObjData.Normals[vnIdx].Y;
                    Vertex.nz = InObjData.Normals[vnIdx].Z;
                }

                uint32 NewIndex = static_cast<uint32>(OutVertices.size());
                OutVertices.push_back(Vertex);
                OutIndices.push_back(NewIndex);

                UniqueVertex[Key] = NewIndex;
            }
        }
    }


    //for (size_t i = 0; i < InObjData.Faces.size(); i++)
    //{
    //    for (size_t j = 0; j < InObjData.Faces[i].size(); j++)
    //    {
    //        FVertexData Vertex{};

    //        int vIdx = InObjData.Faces[i][j].v - 1;
    //        int vtIdx = InObjData.Faces[i][j].vt - 1;
    //        int vnIdx = InObjData.Faces[i][j].vn - 1;

    //        if (vIdx >= 0 && vIdx < static_cast<int>(InObjData.Positions.size()))
    //        {
    //            Vertex.x = InObjData.Positions[vIdx].X;
    //            Vertex.y = InObjData.Positions[vIdx].Y;
    //            Vertex.z = InObjData.Positions[vIdx].Z;
    //        }

    //        if (vtIdx >= 0 && vtIdx < static_cast<int>(InObjData.TexCoords.size()))
    //        {
    //            Vertex.u = InObjData.TexCoords[vtIdx].X;
    //            Vertex.v = InObjData.TexCoords[vtIdx].Y;
    //        }

    //        if (vnIdx >= 0 && vnIdx < static_cast<int>(InObjData.Normals.size()))
    //        {
    //            Vertex.nx = InObjData.Normals[vnIdx].X;
    //            Vertex.ny = InObjData.Normals[vnIdx].Y;
    //            Vertex.nz = InObjData.Normals[vnIdx].Z;
    //        }

    //        OutIndices.push_back(static_cast<uint32>(OutVertices.size()));
    //        OutVertices.push_back(Vertex);
    //    }
    //}

    OutSections = InObjData.Sections;

    return true;
}

bool FObjParser::SaveMeshToBinary(const char* OutFilePath, uint64 InSourceHash, const TArray<FVertexData>& InVertices, TArray<uint32>& InIndices, TArray<FMeshSection>& InSections)
{
    std::ofstream File;
    File.open(OutFilePath, std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    Header.VertexCount = static_cast<uint32>(InVertices.size());
    Header.IndexCount = static_cast<uint32>(InIndices.size());
    Header.SectionCount = static_cast<uint32>(InSections.size());
    Header.SourceHash = InSourceHash;

    File.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

    const size_t VertexDataSize = sizeof(FVertexData) * InVertices.size();
    File.write(reinterpret_cast<const char*>(InVertices.data()), VertexDataSize);

    const size_t IndexDataSize = sizeof(uint32) * InIndices.size();
    File.write(reinterpret_cast<const char*>(InIndices.data()), IndexDataSize);

    for (const auto& Section : InSections)
    {
        uint32 NameLen = static_cast<uint32>(Section.SectionName.size());
        File.write(reinterpret_cast<const char*>(&NameLen), sizeof(uint32));

        File.write(Section.SectionName.data(), NameLen);

        File.write(reinterpret_cast<const char*>(&Section.StartIndex), sizeof(uint32));
        File.write(reinterpret_cast<const char*>(&Section.IndexCount), sizeof(uint32));
    }

    File.close();
    return true;
}

bool FObjParser::LoadMeshFromBinary(const char* InFilePath, TArray<FVertexData>& OutVertices, TArray<uint32>& OutIndices, TArray<FMeshSection>& OutSections)
{
    std::ifstream File;
    File.open(InFilePath, std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    File.read(reinterpret_cast<char*>(&Header), sizeof(Header));

    OutVertices.resize(Header.VertexCount);
    const size_t VertexDataSize = sizeof(FVertexData) * Header.VertexCount;
    File.read(reinterpret_cast<char*>(OutVertices.data()), VertexDataSize);

    OutIndices.resize(Header.IndexCount);
    const size_t IndexDataSize = sizeof(uint32) * Header.IndexCount;
    File.read(reinterpret_cast<char*>(OutIndices.data()), IndexDataSize);

    OutSections.resize(Header.SectionCount);
    for (int i = 0; i < Header.SectionCount; i++)
    {
        uint32 NameLen = 0;
        File.read(reinterpret_cast<char*>(&NameLen), sizeof(uint32));

        OutSections[i].SectionName.resize(NameLen);
        File.read(&OutSections[i].SectionName[0], NameLen);

        File.read(reinterpret_cast<char*>(&OutSections[i].StartIndex), sizeof(uint32));
        File.read(reinterpret_cast<char*>(&OutSections[i].IndexCount) , sizeof(uint32));
    }    

    File.close();
    return true;
}

bool FObjParser::LoadMtl(const char* InFilePath, TArray<FMtlData>& OutResult)
{
    std::ifstream File(InFilePath);
    if (!File.is_open())
    {
        return false;
    }

    FMtlData CurrentMtl;
    bool bHasMat = false;

    FString Line;
    while (std::getline(File, Line))
    {
        if (Line.empty() || Line[0] == '#')
        {
            continue;
        }

        std::stringstream ss(Line);
        FString Prefix;
        ss >> Prefix;

        if (Prefix == "newmtl")
        {
            if (bHasMat)
            {
                OutResult.push_back(CurrentMtl);
            }
            CurrentMtl = FMtlData();
            ss >> CurrentMtl.MaterialName;

            bHasMat = true;
        }
        else if (Prefix == "Kd")
        {
            ss >> CurrentMtl.Kd.X >> CurrentMtl.Kd.Y >> CurrentMtl.Kd.Z;
        }
        else if (Prefix == "map_Kd")
        {
            ss >> CurrentMtl.map_Kd;
        }
        else if (Prefix == "d")
        {
            ss >> CurrentMtl.d; 
        }
    }
    
    if (bHasMat)
    {
        OutResult.push_back(CurrentMtl);
    }

    return true;
}

bool FObjParser::ValidateBinary(const char* InBinFilePath, const char* InObjFilePath)
{
    std::ifstream BinFile;
    BinFile.open(InBinFilePath, std::ios::binary);
    if (!BinFile.is_open())
    {
        return false;
    }

    FMeshFileHeader Header;
    BinFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));

    // Check Magic number
    if (Header.Magic != 0x4D455348)
    {
        return false;
    }

    uint64 ObjHash = ComputeFileHash(std::filesystem::path (InObjFilePath));

    return Header.SourceHash == ObjHash;
}

FObjIndex FObjParser::ParseFaceToken(const FString& Token)
{
    FObjIndex Result;
    if (sscanf_s(Token.c_str(), "%d/%d/%d", &Result.v, &Result.vt, &Result.vn) == 3) return Result;
    if (sscanf_s(Token.c_str(), "%d//%d", &Result.v, &Result.vn) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d/%d", &Result.v, &Result.vt) == 2) return Result;
    if (sscanf_s(Token.c_str(), "%d", &Result.v) == 1) return Result;

    return Result;
}

uint64 FObjParser::ComputeFileHash(const std::filesystem::path& FilePath)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        return 0;
    }

    constexpr uint64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64 FNV_PRIME = 1099511628211ULL;

    uint64 Hash = FNV_OFFSET_BASIS;
    char Buffer[4096]; // 4KB buffer

    while (File.read(Buffer, sizeof(Buffer)) || File.gcount() > 0)
    {
        std::streamsize BytesRead = File.gcount();
        for (std::streamsize i = 0; i < BytesRead; i++)
        {
            Hash ^= static_cast<uint8_t>(Buffer[i]);
            Hash *= FNV_PRIME;
        }

    }

    return Hash;
}

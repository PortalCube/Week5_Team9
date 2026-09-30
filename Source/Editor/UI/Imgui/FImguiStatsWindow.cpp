#include "FImguiStatsWindow.h"
#include "FImguiManager.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Engine/FTimeManager.h"

#include <algorithm>

double FImguiStatsWindow::GetStat(const FName& Name, size_t Range) const
{
    if (Range == 0) { return 0.0; }

    FStatsManager& Stat = FStatsManager::Get();
    const FStatEntry* Entry = Stat.GetEntry(Name);

    if (!Entry) { return 0.0; }

    double Result = 0.0;

    size_t Size = Entry->Value.size();
    Range = std::min(Size, Range);

    for (int32 i = 0; i < Range; ++i)
    {
        Result += Entry->Value[Size - i - 1];
    }

    Result /= Range;

    return Result;
}

void FImguiStatsWindow::Process(FEditor& Editor, float InDeltaTime) {

    if (bOpenMemory)
    {
        DrawStatsMemory();
        DrawGPUStatsMemory();
    }
    if (bOpenFPS)
    {
        DrawStatsFPS();
    }
    if (bOpenUnit)
    {
        DrawUnits();
    } 
}

void FImguiStatsWindow::DrawStatsMemory()
{
    FVector4 Color(0.0f, 255.0f, 0.0f, 255.0f);
    FVector4 OddRowColor(30.0f, 30.0f, 30.0f, 200.0f);
    FVector4 EvenRowColor(10.0f, 10.0f, 10.0f, 200.0f);

    const float Width = 350.0f;
    const float RowHeight = 20.0f;

    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 Pos = {
        ViewportPos.x + ViewportSize.x * 0.2f,
        ViewportPos.y + ViewportSize.y * 0.2f
    };

    CpuY = Pos.y;

    DrawRow(DrawList, ImVec2(Pos.x, Pos.y - 45.0f), CpuY,
        Width, RowHeight, 240.0f,
        "[CPU Memory]", "",
        0, FVector4(255.0f, 255.0f, 255.0f, 255.0f), FVector4(0.0f, 0.0f, 0.0f, 200.0f));

    DrawRow(DrawList, ImVec2(Pos.x, Pos.y - 20.0f), CpuY,
        Width, RowHeight, 240.0f,
        "Memory Counters", "UsedMax",
        0, FVector4(255.0f, 165.0f, 0.0f, 255.0f), FVector4(0.0f, 0.0f, 0.0f, 200.0f));

    // CPU
    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "CPU Memory", "%.2f MB",
        static_cast<double>(FStatsManager::Get().GetProcessMemoryUsed())
        / (1024.0 * 1024.0), Color, OddRowColor);
    // Ram
    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "Ram Used", "%.2f GB",
        static_cast<double>(FStatsManager::Get().GetSystemMemoryUsed())
        / (1024.0 * 1024.0 * 1024.0), Color, EvenRowColor);   // GB 단위 변환 필요
    // Ram Available
    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "Ram Available", "%.2f GB",
        static_cast<double>(FStatsManager::Get().GetSystemMemoryAvailable())
        / (1024.0 * 1024.0 * 1024.0), Color, OddRowColor);   // GB 단위 변환 필요

    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "Total Memory Pool", "%.2f MB",
        GetStat(FName("MemoryPool")) //, 30.0f);
        / (1024.0 * 1024.0), Color, EvenRowColor);

    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "Memory Pool Used", "%.2f MB",
        GetStat(FName("MemoryPoolUsed")) //, 10.0f);
        / (1024.0 * 1024.0), Color, OddRowColor);

    DrawRow(DrawList, Pos, CpuY,
        Width, RowHeight, 240.0f,
        "Memory Pool Free", "%.2f MB",
        GetStat(FName("MemoryPoolFree")) //, 30.0f);
        / (1024.0 * 1024.0), Color, EvenRowColor);
}

void FImguiStatsWindow::DrawGPUStatsMemory()
{
    FVector4 Color(0, 255.0f, 0.0f, 255.0f);
    FVector4 OddRowColor(30.0f, 30.0f, 30.0f, 200.0f);
    FVector4 EvenRowColor(10.0f, 10.0f, 10.0f, 200.0f);

    const float Width = 350.0f;
    const float RowHeight = 20.0f;

    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 Pos = {
        ViewportPos.x + ViewportSize.x * 0.2f + Width,
        ViewportPos.y + ViewportSize.y * 0.2f
    };

    GpuY = Pos.y;

    DrawRow(DrawList, ImVec2(Pos.x, Pos.y - 45.0f), GpuY,
        Width, RowHeight, 240.0f,
        "[GPU Memory]", "",
        0, FVector4(255.0f, 255.0f, 255.0f, 255.0f), FVector4(0.0f, 0.0f, 0.0f, 200.0f));

    DrawRow(DrawList, ImVec2(Pos.x, Pos.y - 20.0f), GpuY,
        Width, RowHeight, 240.0f,
        "Memory Counters", "UsedMax",
        0, FVector4(255.0f, 165.0f, 0.0f, 255.0f), FVector4(0.0f, 0.0f, 0.0f, 200.0f));
    // GPU
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "GPU Memory Used", "%.2f MB",
        static_cast<double>(FStatsManager::Get().GetGPUMemoryUsed())
        / (1024.0 * 1024.0), Color, OddRowColor);
    // GPU Available
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "GPU Memory Available", "%.2f GB",
        static_cast<double>(FStatsManager::Get().GetGPUMemoryBudget())
        / (1024.0 * 1024.0 * 1024.0), Color, EvenRowColor);

    // Vetex Shader
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "VertexShader", "%.2f MB",
        GetStat(FName("VertexShaderMemory"))
        / (1024.0 * 1024.0), Color, OddRowColor);
    // Pixel Shader
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "Pixel Shader", "%.2f MB",
        GetStat(FName("PixelShaderMemory"))
        / (1024.0 * 1024.0), Color, EvenRowColor);
    // Texture
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "Texture", "%.2f MB",
        GetStat(FName("TextureMemory"))
        / (1024.0 * 1024.0), Color, OddRowColor);

    // Static Mesh
    DrawRow(DrawList, Pos, GpuY,
        Width, RowHeight, 240.0f,
        "Static Mesh", "%.2f MB",
        GetStat(FName("StaticMeshMemory")) //, 10.0f);
        / (1024.0 * 1024.0), Color, EvenRowColor);
}

void FImguiStatsWindow::DrawStatsFPS()
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const float Width = 500.0f;
    const float RowHeight = 20.0f;

    const ImVec2 FPSPos = {
        ViewportPos.x + ViewportSize.x - 180.0f,
        ViewportPos.y + ViewportSize.y * 0.25f
    };

    float Y = FPSPos.y;
    FVector4 FPSColor(0.0f, 255.0f, 255.0f, 255.0f);
    FVector4 TransColor(0.0f, 0.0f, 0.0f, 128.0f);
    char Buffer[64];

    double DeltaTime = FTimeManager::GetDeltaTime();

    DrawRow(DrawList, FPSPos, Y, Width, RowHeight, 0.0f,
        "", "%.2f FPS", 1.0f / DeltaTime, FPSColor, TransColor);

    DrawRow(DrawList, FPSPos, Y, Width, RowHeight, 0.0f,
        "", "%.2f ms", 1000.0f * DeltaTime, FPSColor, TransColor);
}

void FImguiStatsWindow::DrawUnits()
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const float Width = 180.0f;
    const float RowHeight = 20.0f;
    const float ValueOffsetX = 80.0f;
    constexpr double BytesPerMB = 1024.0 * 1024.0;

    // FPS 패널(2줄) 바로 아래에 붙인다.
    const ImVec2 Pos = {
        ViewportPos.x + ViewportSize.x - 180.0f,
        ViewportPos.y + ViewportSize.y * 0.25f + RowHeight * 2.0f
    };

    float Y = Pos.y;
    FVector4 Color(0.0f, 255.0f, 255.0f, 255.0f);
    FVector4 TransColor(0.0f, 0.0f, 0.0f, 128.0f);

    FStatsManager& Stats = FStatsManager::Get();

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Frame", "%.2f ms", GetStat(FName("Frame")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Game", "%.2f ms", GetStat(FName("Game")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Draw", "%.2f ms", GetStat(FName("Draw")), Color, TransColor);

    // Draw 안에서 드로우 명령을 모으는 시간 (LOD 선택 비용이 여기에 포함된다)
    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Collect", "%.2f ms", GetStat(FName("Collect")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "GPU Time", "%.2f ms", GetStat(FName("GPU Time")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Input", "%.2f ms", GetStat(FName("Input")), Color, TransColor);

    // Mem/Vram은 프레임마다 새로 물어보는 값이라 스탯으로 누적하지 않는다.
    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Mem", "%.0f MB",
        static_cast<double>(Stats.GetProcessMemoryUsed()) / BytesPerMB, Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Vram", "%.0f MB",
        static_cast<double>(Stats.GetGPUMemoryUsed()) / BytesPerMB, Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Draws", "%.0f", GetStat(FName("Draws")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
        "Prims", "%.0f", GetStat(FName("Prims")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "Frustum", "%.2f ms", GetStat(FName("Frustum")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "Occlusion", "%.2f ms", GetStat(FName("Occlusion")) + GetStat(FName("OcclusionSelect"))
            + GetStat(FName("OcclusionRaster")) + GetStat(FName("OcclusionTest")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "OccSelect", "%.2f ms", GetStat(FName("OcclusionSelect")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "OccRaster", "%.2f ms", GetStat(FName("OcclusionRaster")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "OccTest", "%.2f ms", GetStat(FName("OcclusionTest")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "Occluders", "%.0f", GetStat(FName("Occluders")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "Occluded", "%.0f", GetStat(FName("Occluded")), Color, TransColor);

    DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
            "OccSkip", "%.0f", GetStat(FName("OcclusionSkipped")), Color, TransColor);
}

void FImguiStatsWindow::DrawRow(ImDrawList* DrawList,
    const ImVec2& Pos, float& Y,
    const float& Width, const float& RowHeight,
    const float& ValueOffsetX,
    const char* Name, const char* Value, double Data,
    FVector4 Color, FVector4 RowColor)
{
    char Buffer[64];

    // Memory 전체 배경
    DrawList->AddRectFilled(
        ImVec2(Pos.x, Y),
        ImVec2(Pos.x + Width, Y + RowHeight),
        IM_COL32(RowColor.X, RowColor.Y, RowColor.Z, RowColor.W)
    );

    // Vertex Shader
    sprintf_s(Buffer, Value, Data);

    const ImU32 TextColor = IM_COL32(Color.X, Color.Y, Color.Z, Color.W);

    DrawList->AddText(ImVec2(Pos.x, Y), TextColor, Name);
    DrawList->AddText(ImVec2(Pos.x + ValueOffsetX, Y), TextColor, Buffer);

    Y += RowHeight;
}
void FImguiStatsWindow::Toggle(EStatsWindow Window)
{
    switch (Window)
    {
    case EStatsWindow::Memory:
        bOpenMemory = !bOpenMemory;
        break;

    case EStatsWindow::FPS:
        bOpenFPS = !bOpenFPS;
        break;

    case EStatsWindow::Unit:
        bOpenUnit = !bOpenUnit;
        break;
    }

    RefreshCollecting();
}

void FImguiStatsWindow::SetClose()
{
    bOpenMemory = false;
    bOpenFPS = false;
    bOpenUnit = false;

    RefreshCollecting();
}

void FImguiStatsWindow::RefreshCollecting()
{
    // Cycle/Counter 스탯은 Unit 패널에서만 쓰므로 그 패널을 따라간다.
    // Memory 패널이 읽는 값은 Memory 타입이라 항상 수집되고 있어 손댈 필요가 없다.
    // FPS 패널은 FTimeManager를 직접 읽어 스탯을 쓰지 않는다.
    FStatsManager::Get().SetUnitStatsEnabled(bOpenUnit);
}

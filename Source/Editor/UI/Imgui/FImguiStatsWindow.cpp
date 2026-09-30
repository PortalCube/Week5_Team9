#include "FImguiStatsWindow.h"
#include "FImguiManager.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Engine/FTimeManager.h"

#include "ThirdParty/Imgui/implot.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float FPSHistoryDuration = 3.0f;
    constexpr float FPSSampleInterval = 0.05f;
    constexpr float FPSSmoothingFactor = 0.15f;
    constexpr float FPSGraphPaddingRatio = 0.15f;
    constexpr float FPSLineWeight = 2.0f;
    constexpr float StatsPanelWidth = 220.0f;
    constexpr float StatsGraphHeight = 60.0f;
    constexpr float StatsRowHeight = 20.0f;
}

FImguiStatsWindow::ScrollingBuffer::ScrollingBuffer(size_t InMaxSize)
    : MaxSize(std::max<size_t>(InMaxSize, 1))
{
    Data.reserve(MaxSize);
}

void FImguiStatsWindow::ScrollingBuffer::AddPoint(float X, float Y)
{
    if (Data.size() < MaxSize)
    {
        Data.emplace_back(X, Y);
        return;
    }

    Data[Offset] = ImVec2(X, Y);
    Offset = (Offset + 1) % MaxSize;
}

void FImguiStatsWindow::ScrollingBuffer::Reset()
{
    Data.clear();
    Offset = 0;
}


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

    if (Editor.bShowBenchmark)
    {
        DrawPickingStatsOverlay(Editor);
    }

    if (bOpenMemory)
    {
        DrawStatsMemory();
        DrawGPUStatsMemory();
    }
    if (bOpenFPS)
    {
        UpdateFPSHistory(InDeltaTime);
        DrawStatsFPSGraph();
        DrawStatsFPS(InDeltaTime);
    }
    if (bOpenUnit)
    {
        DrawUnits();
    } 
}

void FImguiStatsWindow::DrawPickingStatsOverlay(const FEditor& Editor)
{
    const double DeltaTime = FTimeManager::GetDeltaTime();
    const ImVec2 ViewportPos = ImGui::GetWindowPos();
    const ImVec2 ViewportSize = ImGui::GetWindowSize();

    const int ResolutionX = static_cast<int>(ViewportSize.x);
    const int ResolutionY = static_cast<int>(ViewportSize.y - ImGui::GetFrameHeight());
    const int FPS = DeltaTime > 0.0 ? static_cast<int>(1.0 / DeltaTime) : 0;
    const double FrameMs = DeltaTime * 1000.0;

    char Buffer[256];
    snprintf(Buffer, sizeof(Buffer),
        "Resolution : %dx%d\nFPS : %d (%.2f ms)\nPicking Time %.4f ms : Num Attempts %d : Accumulated Time %.4f ms",
        ResolutionX, ResolutionY, FPS, FrameMs,
        Editor.LastPickingMs, Editor.PickingAttempts, Editor.AccumulatedPickingMs);

    const ImVec2 Pos(ViewportPos.x + 12.0f, ViewportPos.y + ImGui::GetFrameHeight() + 6.0f);
    constexpr float FontSize = 26.0f;
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // 밝은 장면에서도 읽히도록 그림자를 먼저 그린다.
    DrawList->AddText(ImGui::GetFont(), FontSize, ImVec2(Pos.x + 2.0f, Pos.y + 2.0f),
        IM_COL32(0, 0, 0, 220), Buffer);
    DrawList->AddText(ImGui::GetFont(), FontSize, Pos,
        IM_COL32(0, 255, 0, 255), Buffer);
}

void FImguiStatsWindow::UpdateFPSHistory(float DeltaTime)
{
    if (DeltaTime <= 0.0f || !std::isfinite(DeltaTime))
    {
        return;
    }

    FPSHistoryTime += DeltaTime;
    FPSSampleAccumulator += DeltaTime;

    const float CurrentFPS = 1.0f / DeltaTime;
    if (!bHasSmoothedFPS)
    {
        SmoothedFPS = CurrentFPS;
        bHasSmoothedFPS = true;
    }
    else
    {
        SmoothedFPS += FPSSmoothingFactor * (CurrentFPS - SmoothedFPS);
    }

    if (FPSHistory.IsEmpty() || FPSSampleAccumulator >= FPSSampleInterval)
    {
        FPSHistory.AddPoint(FPSHistoryTime, SmoothedFPS);
        FPSSampleAccumulator = 0.0f;
    }
}

void FImguiStatsWindow::ResetFPSHistory()
{
    FPSHistory.Reset();
    FPSHistoryTime = 0.0f;
    FPSSampleAccumulator = 0.0f;
    SmoothedFPS = 0.0f;
    bHasSmoothedFPS = false;
}

void FImguiStatsWindow::DrawStatsFPSGraph()
{
    if (FPSHistory.IsEmpty())
    {
        return;
    }

    float MinFPS = std::numeric_limits<float>::max();
    float MaxFPS = std::numeric_limits<float>::lowest();
    const float HistoryStart = FPSHistoryTime - FPSHistoryDuration;

    for (const ImVec2& Point : FPSHistory.Data)
    {
        if (Point.x >= HistoryStart)
        {
            MinFPS = std::min(MinFPS, Point.y);
            MaxFPS = std::max(MaxFPS, Point.y);
        }
    }

    if (MinFPS > MaxFPS)
    {
        return;
    }

    const float Range = std::max(MaxFPS - MinFPS, 1.0f);
    const float Padding = Range * FPSGraphPaddingRatio;
    const float YMin = std::max(0.0f, MinFPS - Padding);
    const float YMax = MaxFPS + Padding;

    const ImVec2 FPSGraphSize(StatsPanelWidth, StatsGraphHeight);
    const ImVec2 ViewportPos = ImGui::GetWindowPos();
    const ImVec2 ViewportSize = ImGui::GetWindowSize();
    const ImVec2 GraphPos(
        ViewportPos.x + ViewportSize.x - StatsPanelWidth,
        ViewportPos.y + ImGui::GetFrameHeight());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowPos(GraphPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(FPSGraphSize);
    constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin("##FPSGraphOverlay", nullptr, WindowFlags))
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
        if (ImPlot::BeginPlot("##FPSHistory", FPSGraphSize, ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs)) {
            constexpr ImPlotAxisFlags AxisFlags = ImPlotAxisFlags_NoTickLabels
                | ImPlotAxisFlags_NoTickMarks
                | ImPlotAxisFlags_NoGridLines;
            ImPlot::SetupAxes(nullptr, nullptr, AxisFlags, AxisFlags);
            ImPlot::SetupAxisLimits(ImAxis_X1, HistoryStart, FPSHistoryTime, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, YMin, YMax, ImGuiCond_Always);

            ImPlotSpec Spec;
            Spec.Offset = static_cast<int>(FPSHistory.Offset);
            Spec.Stride = sizeof(ImVec2);
            Spec.FillAlpha = 1.0f;
            Spec.LineWeight = FPSLineWeight;

            ImPlot::PlotLine("FPS", &FPSHistory.Data[0].x, &FPSHistory.Data[0].y,
                static_cast<int>(FPSHistory.Data.size()), Spec);

            char RangeBuffer[64];
            sprintf_s(RangeBuffer, "[FPS] Min %.1f  Max %.1f", MinFPS, MaxFPS);

            const ImVec2 PlotPos = ImPlot::GetPlotPos();
            const ImVec2 PlotSize = ImPlot::GetPlotSize();
            const ImVec2 TextSize = ImGui::CalcTextSize(RangeBuffer);
            constexpr float TextPadding = 4.0f;
            const ImVec2 TextPos(
                PlotPos.x + TextPadding,
                PlotPos.y + PlotSize.y - TextSize.y - TextPadding);

            ImGui::GetWindowDrawList()->AddText(
                TextPos,
                ImGui::GetColorU32(ImGuiCol_Text),
                RangeBuffer);

            ImPlot::EndPlot();
        }
        ImPlot::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();

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

void FImguiStatsWindow::DrawStatsFPS(float DeltaTime)
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const FVector4 FPSColor(0.0f, 255.0f, 255.0f, 255.0f);
    const FVector4 TransColor(0.0f, 0.0f, 0.0f, 128.0f);

    const double Fps = DeltaTime > 0.0 ? 1.0 / DeltaTime : 0.0;
    const double FrameMs = 1000.0 * DeltaTime;

    char Buffer[64];
    sprintf_s(Buffer, "%.2f FPS (%.2fms)", Fps, FrameMs);

    const ImVec2 FPSPos(
        ViewportPos.x + ViewportSize.x - StatsPanelWidth,
        ViewportPos.y + ImGui::GetFrameHeight() + StatsGraphHeight);

    DrawList->AddRectFilled(
        FPSPos,
        ImVec2(FPSPos.x + StatsPanelWidth, FPSPos.y + StatsRowHeight),
        IM_COL32(TransColor.X, TransColor.Y, TransColor.Z, TransColor.W));
    DrawList->AddText(
        FPSPos,
        IM_COL32(FPSColor.X, FPSColor.Y, FPSColor.Z, FPSColor.W),
        Buffer);
}

void FImguiStatsWindow::DrawUnits()
{
    ImVec2 ViewportPos = ImGui::GetWindowPos();
    ImVec2 ViewportSize = ImGui::GetWindowSize();
    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const float Width = StatsPanelWidth;
    const float RowHeight = StatsRowHeight;
    const float ValueOffsetX = 80.0f;
    constexpr double BytesPerMB = 1024.0 * 1024.0;

    // FPS가 켜져 있으면 그래프 아래 한 줄 바로 밑에, 아니면 기존 위치에 붙인다.
    const float StatsTop = ViewportPos.y + ImGui::GetFrameHeight() + StatsGraphHeight;
    const ImVec2 Pos = {
        ViewportPos.x + ViewportSize.x - StatsPanelWidth,
        bOpenFPS ? StatsTop + StatsRowHeight : ViewportPos.y + ViewportSize.y * 0.25f
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

    /*DrawRow(DrawList, Pos, Y, Width, RowHeight, ValueOffsetX,
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
            "OccSkip", "%.0f", GetStat(FName("OcclusionSkipped")), Color, TransColor);*/
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
        if (bOpenFPS)
        {
            ResetFPSHistory();
        }
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
    ResetFPSHistory();

    RefreshCollecting();
}

void FImguiStatsWindow::RefreshCollecting()
{
    // Cycle/Counter 스탯은 Unit 패널에서만 쓰므로 그 패널을 따라간다.
    // Memory 패널이 읽는 값은 Memory 타입이라 항상 수집되고 있어 손댈 필요가 없다.
    // FPS 패널은 FTimeManager를 직접 읽어 스탯을 쓰지 않는다.
    FStatsManager::Get().SetUnitStatsEnabled(bOpenUnit);
}

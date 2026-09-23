#include "FImguiStatsWindow.h"
#include "FImguiManager.h"
#include "Runtime/CoreUObject/FStatsManager.h"


void FImguiStatsWindow::Process(FEditor& Editor, float InDeltaTime) {

    if (bOpenMemory) {
        DrawMemory();
    }

    DeltaTime = InDeltaTime;
    
}

void FImguiStatsWindow::DrawMemory() {
    ImGui::Begin("STAT MEMORY");

    auto& Stats = FStatsManager::Get();

    const auto ToMB = [](size_t Bytes) {
        return Bytes / (1024.0 * 1024.0);
    };

    ImGui::Text("CPU");
    ImGui::Text(
        "  CPU Memory Used: %.2f MB",
        ToMB(Stats.GetProcessMemoryUsed()));

    ImGui::Separator();

    ImGui::Text("SYSTEM");
    ImGui::Text(
        "  RAM Used: %.2f GB",
        Stats.GetSystemMemoryUsed() / (1024.0 * 1024.0 * 1024.0));

    ImGui::Text(
        "  RAM Available: %.2f GB",
        Stats.GetSystemMemoryAvailable() / (1024.0 * 1024.0 * 1024.0));

    ImGui::Separator();

    ImGui::Text("GPU");
    ImGui::Text(
        "  GPU Memory Used: %.2f MB",
        ToMB(Stats.GetGPUMemoryUsed()));

    ImGui::Text(
        "  GPU Memory Budget: %.2f MB",
        ToMB(Stats.GetGPUMemoryBudget()));

    ImGui::Separator();

    ImGui::Text("Shader");
    ImGui::Text(
        "  VertexShader Memory Used: %.2f MB",
        ToMB(Stats.GetVertexShaderMemoryUsed()));

    ImGui::Text(
        "  PixelShader Memory Budget: %.2f MB",
        ToMB(Stats.GetPixelShaderMemoryUsed()));

    ImGui::Text("Texture");
    ImGui::Text(
        "  Texture Memory Used: %.2f MB",
        ToMB(Stats.GetTextureMemoryUsed()));

    ImGui::End();
}

void FImguiStatsWindow::DrawFPS() {
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::Begin("##FPS",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings
        );

    ImGui::Text("%.2f FPS", 1.0f / DeltaTime);
    ImGui::Text("%.2f ms", 1000.0f * DeltaTime);

    ImGui::End();
}
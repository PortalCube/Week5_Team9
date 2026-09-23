#include "FImguiControlPanelWindow.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Editor/Core/EditorConstant.h"
#include <Windows.h>
#include <ShlObj.h>
#include <filesystem>

namespace
{
    // <디렉토리>/<이름>.Scene 형식으로 씬 경로 생성
    FString MakeScenePath(const char* SceneName)
    {
        PWSTR UserPath = nullptr;

        if (FAILED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &UserPath)))
            return "";

        std::filesystem::path Path = UserPath;
        CoTaskMemFree(UserPath);

        Path /= "week3_team9";
        Path /= "SceneData";
        Path /= FString(SceneName) + ".Scene";

        return Path.string();
    }
}

void FImguiControlPanelWindow::Process(FEditor& Editor)
{
    const uint64 Count = UObject::GetTotalAllocationCount();
    const uint64 Bytes = UObject::GetTotalAllocationBytes();
    ImGui::Begin("Jungle Control Panel");

    ImGui::Text("Hello Jungle World!");
    //FPS 표시
    ImGui::Text("FPS %.0f (%.0f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    //메모리 사용 표시
    ImGui::Text("Live UObjects : %llu, UObject Memory: %llu bytes (%.2f KiB)", static_cast<unsigned long long>(Count), static_cast<unsigned long long>(Bytes), static_cast<double>(Bytes) / 1024.0);
    ImGui::Separator();

    //액터 스폰
    ActorSpawnSetting(Editor);
    // 그리드 설정
    GridSetting(Editor);
    // 뷰포트 렌더 모드 및 쇼 플래그 설정
    RenderModeAndShowFlagSetting(Editor);
    ImGui::Separator();
    //카메라 
    CameraSetting(Editor);
    ImGui::Separator();
    //전역조명
    DirectionLightSetting(Editor);
    ImGui::End();
}

void FImguiControlPanelWindow::ActorSpawnSetting(FEditor& Editor)
{
    static UClass* SelectedActorClass = EditorConstant::SpawnableActors[0];
    const char* PreviewValue = SelectedActorClass->GetUClassName().c_str();

    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::BeginCombo("##Actor", PreviewValue))
    {
        for (const auto Item : EditorConstant::SpawnableActors)
        {
            const bool bIsSelected = SelectedActorClass == Item;
            const char* ItemDisplayName = Item->GetUClassName().c_str();
            if (ImGui::Selectable(ItemDisplayName, bIsSelected))
            {
                SelectedActorClass = Item;
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text("Actor");

    float MinLocation = Editor.State.GetSpawnActorMinLocation();
    float MaxLocation = Editor.State.GetSpawnActorMaxLocation();
    ImGui::SetNextItemWidth(40.0f);
    ImGui::Text("Min");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::DragFloat("##SpawnMinLocation", &MinLocation, 0.1f, -100.0f, 100.0f, "%.1f"))
    {
        Editor.State.SetSpawnActorMinLocation(MinLocation);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40.0f);
    ImGui::Text("Max");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::DragFloat("##SpawnMaxLocation", &MaxLocation, 0.1f, -100.0f, 100.0f, "%.1f"))
    {
        Editor.State.SetSpawnActorMaxLocation(MaxLocation);
    }
    ImGui::SameLine();
    ImGui::Text("Spawn Location");

    static int spawnCount = 1;
    if (ImGui::Button("Spawn"))
    {
        const int Count = (spawnCount < 1) ? 1 : spawnCount;
        Editor.SpawnActorToCurrentScene(SelectedActorClass, Count);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputInt("##SpawnCount", &spawnCount);
    ImGui::SameLine();
    ImGui::Text("Number of spawn");

}


    // 그리드 설정
void FImguiControlPanelWindow::GridSetting(FEditor& Editor)
{
    FEditorViewportClient* Viewport = Editor.GetActiveViewport();
    if (!Viewport) { return; }

    float CellSize = Viewport->GetGrid().GetCellSize();
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::DragFloat("##GridCellSize", &CellSize, 0.05f, 0.1f, 15.0f, "%.2f"))
    {
        Viewport->GetGrid().SetCellSize(CellSize);
    }
    ImGui::SameLine();
    ImGui::Text("Grid Cell Size");
}

void FImguiControlPanelWindow::RenderModeAndShowFlagSetting(FEditor& Editor)
{
    
    FEditorViewportClient* ActiveViewport = Editor.GetActiveViewport();
    if (ActiveViewport)
    {
        // 뷰 모드 드롭박스
        int CurrentViewMode = static_cast<int>(ActiveViewport->ViewMode);
        const char* ViewModes[] = { "Lit", "Unlit", "Wireframe" };
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::Combo("##ViewMode", &CurrentViewMode, ViewModes, IM_ARRAYSIZE(ViewModes)))
        {
            ActiveViewport->ViewMode = static_cast<EViewModeIndex>(CurrentViewMode);
        }
        ImGui::SameLine();
        ImGui::Text("View Mode");

        // 쇼 플래그 드롭박스
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::BeginCombo("##ShowFlags", "Show Flags"))
        {
            bool bPrimitives = ActiveViewport->HasShowFlag(EEngineShowFlags::SF_Primitives);
            if (ImGui::Checkbox("Primitives", &bPrimitives))
            {
                ActiveViewport->ToggleShowFlag(EEngineShowFlags::SF_Primitives);
            }

            bool bBillboardText = ActiveViewport->HasShowFlag(EEngineShowFlags::SF_BillboardText);
            if (ImGui::Checkbox("Billboard Text", &bBillboardText))
            {
                ActiveViewport->ToggleShowFlag(EEngineShowFlags::SF_BillboardText);
            }
            bool bGrid = ActiveViewport->HasShowFlag(EEngineShowFlags::SF_Grid);
            if (ImGui::Checkbox("Grid", &bGrid))
            {
                ActiveViewport->ToggleShowFlag(EEngineShowFlags::SF_Grid);
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::Text("Show Flags");
    }
}

void FImguiControlPanelWindow::CameraSetting(FEditor& Editor)
{
    if (FEditorViewportClient* Viewport = Editor.GetActiveViewport())
    {
        FCamera& Camera = Viewport->ViewportCamera;

        bool bOrthographic =
            (Camera.Projection.ProjectionType == EProjectionType::Orthographic);
        //if (ImGui::Checkbox("Orthogonal", &bOrthographic))
        //{
        //    Camera.Projection.ProjectionType =
        //        bOrthographic ? EProjectionType::Orthographic : EProjectionType::Perspective;
        //}

        float CameraSensitivity = Editor.State.GetCameraSensitivity();
        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragFloat("##Sensitivity", &CameraSensitivity, 0.1f, 0.2f, 2.0f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("Sensitivity");
        Editor.State.SetCameraSensitivity(CameraSensitivity);

        float CameraSpeed = Editor.State.GetCameraSpeed();
        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragFloat("##Speed", &CameraSpeed, 0.1f, 0.2f, 5.0f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("Speed");
        Editor.State.SetCameraSpeed(CameraSpeed);

        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragFloat("##FOV", &Camera.Projection.FOV, 0.1f, 1.0f, 179.0f, "%.1f");
        ImGui::SameLine();
        ImGui::Text("FOV");

        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragFloat3("##CameraLocation", &Camera.Position.X, 0.05f, 0.0f, 0.0f, "%.3f");
        ImGui::SameLine();
        ImGui::Text("Camera Location");

        ImGui::SetNextItemWidth(40.0f);
        ImGui::Text("Pitch");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(50.0f);
        ImGui::DragFloat(
            "##CameraPitch",
            &Camera.Pitch,
            0.5f,
            0.0f,
            0.0f,
            "%.2f"
        );
        ImGui::SameLine();

        ImGui::SetNextItemWidth(40.0f);
        ImGui::Text("Yaw");
        ImGui::SameLine();

        ImGui::SetNextItemWidth(50.0f);
        ImGui::DragFloat(
            "##CameraYaw",
            &Camera.Yaw,
            0.5f,
            0.0f,
            0.0f,
            "%.2f"
        );

        ImGui::SameLine();
        ImGui::Text("Camera Rotation");

        if (ImGui::Button("Reset Camera"))
        {
            Camera.Position = FVector{ -8.0f, 0.0f, 4.0f };
            Camera.Pitch = -20.0f;
            Camera.Yaw = 0.0f;
            Editor.State.SetCameraLocation(Camera.Position);
            Editor.State.SetCameraPitch(Camera.Pitch);
            Editor.State.SetCameraYaw(Camera.Yaw);
        }
    }
}

void FImguiControlPanelWindow::DirectionLightSetting(FEditor& Editor)
{
    ImGui::SeparatorText("Sun Light Control");
    // 엑스축 조명 방향 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.32f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
    ImGui::Button("X", ImVec2(22.0f, 0.0f));
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.85f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##LightDirX", &Editor.GlobalLight.LightDirection.X, -1.0f, 1.0f, "%.2f");
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    ImGui::Text("Light Dir X (Forward/Back)");

    // 와이축 조명 방향 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.75f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.32f, 0.85f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
    ImGui::Button("Y", ImVec2(22.0f, 0.0f));
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.22f, 0.75f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.35f, 0.95f, 0.35f, 1.0f));
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##LightDirY", &Editor.GlobalLight.LightDirection.Y, -1.0f, 1.0f, "%.2f");
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    ImGui::Text("Light Dir Y (Right/Left)");

    // 제트축 조명 방향 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.35f, 0.85f, 1.0f));
    ImGui::Button("Z", ImVec2(22.0f, 0.0f));
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.25f, 0.45f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##LightDirZ", &Editor.GlobalLight.LightDirection.Z, -1.0f, 1.0f, "%.2f");
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    ImGui::Text("Light Dir Z (Up/Down)");

    ImGui::SetNextItemWidth(180.0f);
    ImGui::ColorEdit3("##LightColor", &Editor.GlobalLight.LightColor.X);
    ImGui::SameLine();
    ImGui::Text("Color");

    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##LightIntensity", &Editor.GlobalLight.Intensity, 0.0f, 5.0f, "%.2f");
    ImGui::SameLine();
    ImGui::Text("Intensity");

    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##LightAmbient", &Editor.GlobalLight.AmbientIntensity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::Text("Ambient");
}


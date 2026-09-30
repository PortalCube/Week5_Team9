#include "FImguiControlPanelWindow.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Math/Random.h"
#include "Editor/Core/EditorConstant.h"
#include <Windows.h>
#include <ShlObj.h>
#include <filesystem>

void FImguiControlPanelWindow::Process(FEditor& Editor)
{
    const uint64 Count = UObject::GetTotalAllocationCount();
    const uint64 Bytes = UObject::GetTotalAllocationBytes();
    ImGui::Begin("Jungle Control Panel");

    ImGui::Separator();

    if (ImGui::Button("대회 씬 바로 불러오기"))
    {
        Editor.LoadScene("DefaultScene/Default.scene");
    }

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

    ImGui::Separator();
    BVHDebugSetting(Editor);

    ImGui::Separator();
    RenderStateSort(Editor);

    ImGui::Separator();
	FrameResourceDebugSetting(Editor);

    ImGui::Separator();
    LODSetting(Editor);

    ImGui::End();
}

void FImguiControlPanelWindow::BVHDebugSetting(FEditor& Editor)
{
    ImGui::Text("Picking path");

    if (ImGui::Checkbox("Use BVH (QueryRay)", &Editor.bUseBVHPicking))
    {
        // 경로를 바꾸면 누적치를 섞지 않는다
        Editor.ResetPickingStats();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 해제 시 기존 선형 RayIntersectsMeshes 사용");
    }

    if (ImGui::Checkbox("Use Flattened Triangles", &FRayCastingManager::bUseFlattenedTriangles))
    {
        Editor.ResetPickingStats();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 해제 시 기존 인덱스 방식(Positions[Indices[i]])으로 삼각형 검사");
    }

    if (ImGui::Checkbox("Use Mesh BVH", &FRayCastingManager::bUseMeshBVH))
    {
        Editor.ResetPickingStats();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("체크 해제 시 메시의 모든 삼각형을 선형으로 검사 (위 Flattened 옵션을 따름)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset Stats"))
    {
        Editor.ResetPickingStats();
    }
}

void FImguiControlPanelWindow::RenderStateSort(FEditor& Editor)
{
    ImGui::Text("Render State Sort");
    ImGui::Checkbox("정렬 활성화", &Globals::bSortTest);

    if (ImGui::Button("1000 random spawn"))
    {
        for (int i = 0; i < 1000; ++i)
        {
            uint32 Index = Random::Get<uint32>(0, 4);
            Editor.SpawnActorToCurrentScene(EditorConstant::SpawnableActors[Index]);
        }
    }
}

void FImguiControlPanelWindow::FrameResourceDebugSetting(FEditor& Editor)
{
    ImGui::Text("Frame Resource Debug");

    ImGui::Checkbox("프레임 리소스 사용", &Globals::bUseFrameResources);
}

void FImguiControlPanelWindow::LODSetting(FEditor& Editor)
{
    ImGui::Text("LOD");
    ImGui::Checkbox("LOD 활성화", &Globals::bEnableLOD);

    // -1은 자동 선택. 메시의 LOD 개수를 넘으면 가장 거친 LOD로 고정된다.
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderInt("LOD 고정 (-1: 자동)", &Globals::ForcedLOD, -1, 3);

    ImGui::Checkbox("LOD 색상 표시 (흰/빨/초/파)", &Globals::bShowLODColor);
    ImGui::Text("LOD0 %u | LOD1 %u | LOD2 %u | LOD3 %u",
        Globals::LODDrawCounts[0], Globals::LODDrawCounts[1],
        Globals::LODDrawCounts[2], Globals::LODDrawCounts[3]);
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
            (Camera.GetProjection().GetProjectionType() == EProjectionType::Orthographic);
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

        float FOV = Camera.GetProjection().GetFOV();
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::DragFloat("##FOV", &FOV, 0.1f, 1.0f, 179.0f, "%.1f"))
        {
            Camera.SetFOV(FOV);
        }
        ImGui::SameLine();
        ImGui::Text("FOV");

        FVector CameraPosition = Camera.GetPosition();
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::DragFloat3("##CameraLocation", &CameraPosition.X, 0.05f, 0.0f, 0.0f, "%.3f"))
        {
            Camera.SetPosition(CameraPosition);
        }
        ImGui::SameLine();
        ImGui::Text("Camera Location");

        ImGui::SetNextItemWidth(40.0f);
        ImGui::Text("Pitch");
        ImGui::SameLine();

        float Pitch = Camera.GetPitch();
        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::DragFloat(
            "##CameraPitch",
            &Pitch,
            0.5f,
            0.0f,
            0.0f,
            "%.2f"
        ))
        {
            Camera.SetPitch(Pitch);
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(40.0f);
        ImGui::Text("Yaw");
        ImGui::SameLine();

        float Yaw = Camera.GetYaw();
        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::DragFloat(
            "##CameraYaw",
            &Yaw,
            0.5f,
            0.0f,
            0.0f,
            "%.2f"
        ))
        {
            Camera.SetYaw(Yaw);
        }

        ImGui::SameLine();
        ImGui::Text("Camera Rotation");

        if (ImGui::Button("Reset Camera"))
        {
            Camera.SetPosition(FVector{ -8.0f, 0.0f, 4.0f });
            Camera.SetRotation(-20.0f, 0.0f);
            Editor.State.SetCameraLocation(Camera.GetPosition());
            Editor.State.SetCameraPitch(Camera.GetPitch());
            Editor.State.SetCameraYaw(Camera.GetYaw());
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


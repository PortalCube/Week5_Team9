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
#include "Runtime/Engine/UScene.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
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
	SIMDCullingDebugSetting(Editor);

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

    // 마지막 클릭 광선으로 피킹을 반복해 설정 간 차이만 비교한다.
    // 캐시가 데워져 단발 클릭보다 빠르게 나오므로 상대 비교용이다.
    static int BenchIterations = 1000;
    ImGui::BeginDisabled(!FRayCastingManager::bHasLastPickRay);
    if (ImGui::Button("Benchmark Last Ray"))
    {
        RunPickBenchmark(Editor, BenchIterations);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::DragInt("Iterations", &BenchIterations, 10.0f, 1, 100000);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("뷰포트를 한 번 클릭한 뒤 사용. 결과는 로그에 [PickBench]로 출력");
    }

    // 재빌드/재실행 후에도 완전히 같은 광선으로 비교할 수 있게 파일로 저장한다.
    ImGui::BeginDisabled(!FRayCastingManager::bHasLastPickRay);
    if (ImGui::Button("Save Ray"))
    {
        SavePickRay();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Load Ray"))
    {
        LoadPickRay();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s 에서 광선을 불러와 Benchmark Last Ray에 사용", PickRayFilePath);
    }
}

void FImguiControlPanelWindow::SavePickRay()
{
    std::ofstream File(PickRayFilePath);
    if (!File)
    {
        UE_LOG_WARN("[PickBench] 광선 저장 실패: %s", PickRayFilePath);
        return;
    }

    // float를 문자열로 바꿨다가 되읽어도 비트까지 같도록 유효숫자 9자리로 쓴다
    const FRay& R = FRayCastingManager::LastPickRay;
    File << std::setprecision(9)
        << R.Origin.X << ' ' << R.Origin.Y << ' ' << R.Origin.Z << ' '
        << R.Direction.X << ' ' << R.Direction.Y << ' ' << R.Direction.Z << '\n';

    UE_LOG("[PickBench] 광선 저장: %s", PickRayFilePath);
}

void FImguiControlPanelWindow::LoadPickRay()
{
    std::ifstream File(PickRayFilePath);
    if (!File)
    {
        UE_LOG_WARN("[PickBench] 저장된 광선이 없습니다: %s", PickRayFilePath);
        return;
    }

    FRay R;
    File >> R.Origin.X >> R.Origin.Y >> R.Origin.Z >> R.Direction.X >> R.Direction.Y >> R.Direction.Z;

    if (!File)
    {
        UE_LOG_WARN("[PickBench] 광선 파일 형식이 올바르지 않습니다: %s", PickRayFilePath);
        return;
    }

    FRayCastingManager::LastPickRay = R;
    FRayCastingManager::bHasLastPickRay = true;
    UE_LOG("[PickBench] 광선 불러옴: Origin(%.3f, %.3f, %.3f) Dir(%.3f, %.3f, %.3f)",
        R.Origin.X, R.Origin.Y, R.Origin.Z, R.Direction.X, R.Direction.Y, R.Direction.Z);
}

void FImguiControlPanelWindow::RunPickBenchmark(FEditor& Editor, int Iterations)
{
    if (!FRayCastingManager::bHasLastPickRay || Iterations <= 0) { return; }

    UScene* Scene = Editor.GetCurrentScene();
    FEditorViewportClient* Viewport = Editor.GetActiveViewport();
    const bool bUseBVH = Editor.bUseBVHPicking && Scene;
    if (!bUseBVH && !Viewport) { return; }

    const FRay Ray = FRayCastingManager::LastPickRay;
    TArray<double> Times;
    Times.reserve(Iterations);

    UPrimitiveComponent* HitComponent = nullptr;
    FVector ImpactPoint;
    for (int i = 0; i < Iterations; ++i)
    {
        FRayCastingManager::PickProfile.Reset();
        HitComponent = nullptr;

        FScopeCycleCounter Counter;
        if (bUseBVH)
        {
            Scene->GetSceneBVH().QueryRay(Ray, HitComponent, ImpactPoint);
        }
        else
        {
            FRayCastingManager::RayIntersectsMeshes(
                Ray, Viewport->ViewportCamera, Editor.GetPrimitiveComponents(), HitComponent, ImpactPoint);
        }
        Times.push_back(Counter.Finish());
    }

    std::sort(Times.begin(), Times.end());
    double Sum = 0.0;
    for (double T : Times) { Sum += T; }

    // 작업량은 매번 같으므로 마지막 반복의 값을 쓴다
    const FRayCastingManager::FPickProfile& P = FRayCastingManager::PickProfile;
    UE_LOG("[PickBench] %s x%d | Median %.4f ms | Min %.4f ms | Avg %.4f ms | SceneNodes %u | ObjBox %u | MeshTests %u | MeshNodes %u | Tris %u | Hit UUID %u",
        bUseBVH ? "BVH" : "Linear", Iterations,
        Times[Times.size() / 2], Times.front(), Sum / Times.size(),
        P.SceneNodes, P.ObjectBoxTests, P.MeshTests, P.MeshNodes, P.Triangles,
        HitComponent ? HitComponent->GetUUID() : 0u);
}

void FImguiControlPanelWindow::RenderStateSort(FEditor& Editor)
{
    ImGui::Text("Render State Sort");
    ImGui::Checkbox("정렬 활성화", &Globals::bEnableRenderSort);

    if (ImGui::Button("1000 random spawn"))
    {
        for (int i = 0; i < 1000; ++i)
        {
            uint32 Index = Random::Get<uint32>(0, 4);
            Editor.SpawnActorToCurrentScene(EditorConstant::SpawnableActors[Index]);
        }
    }
}

void FImguiControlPanelWindow::SIMDCullingDebugSetting(FEditor& Editor)
{
    ImGui::Text("SIMD 컬링 Debug");

    ImGui::Checkbox("SIMD 컬링 사용", &Globals::bUseSIMDCulling);
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


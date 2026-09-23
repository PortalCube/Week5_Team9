#pragma once

#include "Runtime/Math/FVector.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/FString.h"

class FConfigArchive;

/// <summary>
/// 에디터에서 필요한 상태들을 담는 클래스입니다.
/// Editor에 값에 대한 멤버 변수가 필요하다면, 여기서 처리 후 저장해주세요.
/// </summary>
class FEditorState
{
public:

	static inline FString DefaultFileName = "editor.ini";

	enum class SplitViewMode {
		SINGLE,
		HORIZONTAL,
		VERTICAL,
		QUAD,
	};

private:
	static constexpr float SaveIntervalSeconds = 5.0f;
	float TimeSinceLastSave = 0.0f;
	bool bDirty = false;

	// Camera
	float CameraSensitivity = 0.5f;
	float CameraSpeed = 5.0f;
	FVector CameraLocation = { -8.0f, 0.0f, 4.0f };
	float CameraYaw = 0.0f;
	float CameraPitch = -20.0f;
	float CameraFOV = 60.0f;

	// Grid
	float GridCellSize = 1.0f;

	// Spawn Actor
	float SpawnActorMinLocation = -3.0f;
	float SpawnActorMaxLocation = 3.0f;

	// Gizmo
	uint8 GizmoMode = 1;
	uint8 GizmoSpace = 0;
	uint32 SelectedActor = -1;

	// Viewport
	SplitViewMode splitViewMode = SplitViewMode::SINGLE;
	float SplitterVRatio=0.5f;
	float SplitterHRatio = 0.5f;
	float SplitterH2Ratio = 0.5f;

public:
	void WriteToFile(FStringView FilePath = DefaultFileName) const;
	void ReadFromFile(FStringView FilePath = DefaultFileName);
	void ResetToDefaults();
	void Tick(float DeltaTime);
	void FlushToFile(FStringView FilePath = DefaultFileName);

	void SetCameraSensitivity(float Value);
	float GetCameraSensitivity() const { return CameraSensitivity; }

	void SetCameraSpeed(float Value);
	float GetCameraSpeed() const { return CameraSpeed; }

	void SetCameraLocation(const FVector& Value);
	const FVector& GetCameraLocation() const { return CameraLocation; }

	void SetCameraYaw(float Value);
	float GetCameraYaw() const { return CameraYaw; }

	void SetCameraPitch(float Value);
	float GetCameraPitch() const { return CameraPitch; }

	void SetCameraFOV(float Value);
	float GetCameraFOV() const { return CameraFOV; }

	void SetGridCellSize(float Value);
	float GetGridCellSize() const { return GridCellSize; }

	void SetSpawnActorMinLocation(float Value);
	float GetSpawnActorMinLocation() const { return SpawnActorMinLocation; }

	void SetSpawnActorMaxLocation(float Value);
	float GetSpawnActorMaxLocation() const { return SpawnActorMaxLocation; }

	void SetGizmoMode(uint8 Value);
	uint8 GetGizmoMode() const { return GizmoMode; }

	void SetGizmoSpace(uint8 Value);
	uint8 GetGizmoSpace() const { return GizmoSpace; }

	void SetSelectedActor(uint32 Value);
	uint32 GetSelectedActor() const { return SelectedActor; }

	void SetSplitter(float SplitterV, float SplitterH, float SplitterH2);
	FVector GetSplitter() const { return FVector(SplitterVRatio, SplitterHRatio, SplitterH2Ratio); }

	void SetSplitMode(SplitViewMode mode);
	SplitViewMode GetSplitMode() const { return splitViewMode; }

};

#include "FEditorState.h"
#include "Editor/Core/FConfigArchive.h"
#include "Runtime/Core/Log.h"
#include "ThirdParty/mIni/ini.h"

void FEditorState::WriteToFile(FStringView FilePath) const
{
	FConfigArchive Archive;

	// Camera
	Archive.SetFloat("Camera", "Sensitivity", CameraSensitivity);
	Archive.SetFloat("Camera", "Speed", CameraSpeed);
	Archive.SetVector("Camera", "Location", CameraLocation);
	Archive.SetFloat("Camera", "Yaw", CameraYaw);
	Archive.SetFloat("Camera", "Pitch", CameraPitch);
	Archive.SetFloat("Camera", "FOV", CameraFOV);

	// Grid
	Archive.SetFloat("Grid", "CellSize", GridCellSize);

	// Spawn Actor
	Archive.SetFloat("spawnactor", "min-location", SpawnActorMinLocation);
	Archive.SetFloat("spawnactor", "max-location", SpawnActorMaxLocation);

	// Gizmo
	Archive.SetUInt32("Gizmo", "Mode", GizmoMode);
	Archive.SetUInt32("Gizmo", "Space", GizmoSpace);
	Archive.SetUInt32("Gizmo", "SelectedActor", SelectedActor);

	// Splitter
	Archive.SetFloat("view", "SplitterV", SplitterVRatio);
	Archive.SetFloat("view", "SplitterH", SplitterHRatio);
	Archive.SetFloat("view", "SplitterH2", SplitterH2Ratio);
	Archive.SetUInt32("view", "viewmode", static_cast<uint32>(splitViewMode));

	
	mINI::INIFile File{ FilePath };
	mINI::INIStructure Structure = Archive.GetConfig();

	File.write(Structure, true);
}

void FEditorState::ReadFromFile(FStringView FilePath)
{
	mINI::INIFile File{ FilePath };
	mINI::INIStructure Structure;

	// 파일을 불러오는데 실패하면 기본값 유지
	if (!File.read(Structure))
	{
		UE_LOG("[FEditorState::ReadFromFile] \"%s\" 파일을 불러오는데 실패했습니다.", FilePath);
		return;
	}

	const FConfigArchive Archive{ Structure };

	// Camera

	if (!Archive.IsEmpty("Camera", "Sensitivity"))
	{
		CameraSensitivity = Archive.GetFloat("Camera", "Sensitivity");
	}

	if (!Archive.IsEmpty("Camera", "Speed"))
	{
		CameraSpeed = Archive.GetFloat("Camera", "Speed");
	}

	if
	(
		!Archive.IsEmpty("Camera", "Location.0") &&
		!Archive.IsEmpty("Camera", "Location.1") &&
		!Archive.IsEmpty("Camera", "Location.2")
	)
	{
		CameraLocation = Archive.GetVector("Camera", "Location");
	}

	if (!Archive.IsEmpty("Camera", "Yaw"))
	{
		CameraYaw = Archive.GetFloat("Camera", "Yaw");
	}

	if (!Archive.IsEmpty("Camera", "Pitch"))
	{
		CameraPitch = Archive.GetFloat("Camera", "Pitch");
	}

	if (!Archive.IsEmpty("Camera", "FOV"))
	{
		CameraFOV = Archive.GetFloat("Camera", "FOV");
	}

	// Grid

	if (!Archive.IsEmpty("Grid", "CellSize"))
	{
		GridCellSize = Archive.GetFloat("Grid", "CellSize");
	}

	// Spawn Actor

	if (!Archive.IsEmpty("spawnactor", "min-location"))
	{
		SpawnActorMinLocation = Archive.GetFloat("spawnactor", "min-location");
	}

	if (!Archive.IsEmpty("spawnactor", "max-location"))
	{
		SpawnActorMaxLocation = Archive.GetFloat("spawnactor", "max-location");
	}

	// Gizmo

	if (!Archive.IsEmpty("Gizmo", "Mode"))
	{
		GizmoMode = static_cast<uint8>(Archive.GetUInt32("Gizmo", "Mode"));
		if (GizmoMode == 0)
		{
			GizmoMode = 1;
		}
	}

	if (!Archive.IsEmpty("Gizmo", "Space"))
	{
		GizmoSpace = static_cast<uint8>(Archive.GetUInt32("Gizmo", "Space"));
	}

	if (!Archive.IsEmpty("Gizmo", "SelectedActor"))
	{
		SelectedActor = Archive.GetUInt32("Gizmo", "SelectedActor");
	}

	// ViewMode

	if (!Archive.IsEmpty("view", "splitterv"))
	{
		SetSplitter(static_cast<float>(Archive.GetFloat("view", "splitterv")),
			static_cast<float>(Archive.GetFloat("view", "splitterh")),
			static_cast<float>(Archive.GetFloat("view", "splitterh2")));
	}
	if (!Archive.IsEmpty("view", "viewmode"))
	{
		SetSplitMode(static_cast<FEditorState::SplitViewMode>(Archive.GetUInt32("view","viewmode")));		
	}
	bDirty = false;
	TimeSinceLastSave = 0.0f;
}

void FEditorState::ResetToDefaults()
{
	*this = FEditorState{};
	bDirty = true;
}

void FEditorState::Tick(float DeltaTime)
{
	TimeSinceLastSave += DeltaTime;
	if (TimeSinceLastSave < SaveIntervalSeconds)
	{
		return;
	}

	TimeSinceLastSave = 0.0f;
	FlushToFile();
}

void FEditorState::FlushToFile(FStringView FilePath)
{
	if (!bDirty)
	{
		return;
	}

	WriteToFile(FilePath);
	bDirty = false;
}

void FEditorState::SetCameraSensitivity(float Value)
{
	if (CameraSensitivity == Value) { return; }
	CameraSensitivity = Value;
	bDirty = true;
}

void FEditorState::SetCameraSpeed(float Value)
{
	if (CameraSpeed == Value) { return; }
	CameraSpeed = Value;
	bDirty = true;
}

void FEditorState::SetCameraLocation(const FVector& Value)
{
	if (CameraLocation == Value) { return; }
	CameraLocation = Value;
	bDirty = true;
}

void FEditorState::SetCameraYaw(float Value)
{
	if (CameraYaw == Value) { return; }
	CameraYaw = Value;
	bDirty = true;
}

void FEditorState::SetCameraPitch(float Value)
{
	if (CameraPitch == Value) { return; }
	CameraPitch = Value;
	bDirty = true;
}

void FEditorState::SetCameraFOV(float Value)
{
	if (CameraFOV == Value) { return; }
	CameraFOV = Value;
	bDirty = true;
}

void FEditorState::SetGridCellSize(float Value)
{
	if (GridCellSize == Value) { return; }
	GridCellSize = Value;
	bDirty = true;
}

void FEditorState::SetSpawnActorMinLocation(float Value)
{
	if (SpawnActorMinLocation == Value) { return; }
	SpawnActorMinLocation = Value;
	bDirty = true;
}

void FEditorState::SetSpawnActorMaxLocation(float Value)
{
	if (SpawnActorMaxLocation == Value) { return; }
	SpawnActorMaxLocation = Value;
	bDirty = true;
}

void FEditorState::SetGizmoMode(uint8 Value)
{
	if (GizmoMode == Value) { return; }
	GizmoMode = Value;
	bDirty = true;
}

void FEditorState::SetGizmoSpace(uint8 Value)
{
	if (GizmoSpace == Value) { return; }
	GizmoSpace = Value;
	bDirty = true;
}

void FEditorState::SetSelectedActor(uint32 Value)
{
	if (SelectedActor == Value) { return; }
	SelectedActor = Value;
	bDirty = true;
}
void FEditorState::SetSplitter(float SplitterV, float SplitterH, float SplitterH2)
{
	if (SplitterVRatio == SplitterV && SplitterHRatio == SplitterH && SplitterH2Ratio == SplitterH2) {return;}
	SplitterVRatio = SplitterV;
	SplitterHRatio = SplitterH;
	SplitterH2Ratio = SplitterH2;
	bDirty = true;
}

void FEditorState::SetSplitMode(SplitViewMode mode)
{
	if (splitViewMode == mode) return;
	splitViewMode = mode;
	bDirty = true;
}


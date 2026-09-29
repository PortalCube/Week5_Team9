#include "FInputManager.h"

#include "Runtime/Core/IntTypes.h"
#include <Windows.h>
#include <cstring>

void FInputManager::BeginFrame()
{
	MouseDelta = CurrentMousePosition - PreviousMousePosition;
	PreviousMousePosition = CurrentMousePosition;

	MouseWheelDelta = AccmulatedWheelData;
	AccmulatedWheelData = 0.0f;
}

void FInputManager::EndFrame()
{
	memcpy(PreviousKeyStates, CurrentKeyStates, sizeof(bool) * MAX_KEYS);
	memcpy(bPreviousMouseState, bCurrentMouseState, sizeof(bool) * MAX_MOUSE_BUTTONS);
}


bool FInputManager::IsKeyPressed(uint32 Key) const
{
	if (Key >= MAX_KEYS)
	{
		return false;
	}
	return CurrentKeyStates[Key];
}

bool FInputManager::IsKeyDown(uint32 Key) const
{
	if (Key >= MAX_KEYS)
	{
		return false;
	}
	return IsKeyPressed(Key) && !IsPrevKeyDown(Key);
}

bool FInputManager::IsKeyUp(uint32 Key) const
{
	if (Key >= MAX_KEYS)
	{
		return false;
	}
	return !IsKeyPressed(Key) && IsPrevKeyDown(Key);
}

bool FInputManager::IsMouseDown(EMouseButton Button) const
{
	size_t MouseIndex = static_cast<size_t>(Button);
	return !bPreviousMouseState[MouseIndex] && bCurrentMouseState[MouseIndex];
}

bool FInputManager::IsMousePressed(EMouseButton Button) const
{
	size_t MouseIndex = static_cast<size_t>(Button);
	return bCurrentMouseState[MouseIndex];
}

bool FInputManager::IsMouseUp(EMouseButton Button) const
{
	size_t MouseIndex = static_cast<size_t>(Button);
	return bPreviousMouseState[MouseIndex] && !bCurrentMouseState[MouseIndex];
}

void FInputManager::SetMousePosition(FVector2 Position)
{
	CurrentMousePosition = Position;
}

void FInputManager::SetMouseButton(EMouseButton Button, bool Value)
{
	size_t MouseIndex = static_cast<size_t>(Button);
	bCurrentMouseState[MouseIndex] = Value;
}

void FInputManager::SetMouseWheel(float Delta)
{
	AccmulatedWheelData += Delta;
}

FVector2 FInputManager::GetMousePosition() const
{
	return CurrentMousePosition;
}

FVector2 FInputManager::GetMouseDelta() const
{
	return MouseDelta;
}

float FInputManager::GetMouseWheelDelta() const
{
	return MouseWheelDelta;
}

void FInputManager::SetKey(uint32 Key, bool Value)
{
	CurrentKeyStates[Key] = Value;
}

bool FInputManager::IsPrevKeyDown(uint32 Key) const
{
	if (Key >= MAX_KEYS)
	{
		return false;
	}
	return PreviousKeyStates[Key];
}

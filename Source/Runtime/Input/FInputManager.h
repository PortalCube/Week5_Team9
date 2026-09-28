#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FVector2.h"

enum class EMouseButton : uint8;

class FInputManager final
{
public:
	static constexpr int32 MAX_KEYS = 256;
	static constexpr int32 MAX_MOUSE_BUTTONS = 5;

	static FInputManager& Get()
	{
		static FInputManager Instance;
		return Instance;
	}
	
	void BeginFrame();
	void EndFrame();

	bool IsKeyDown(uint32 Key) const;
	bool IsKeyPressed(uint32 Key) const;
	bool IsKeyUp(uint32 Key) const;

	bool IsMouseDown(EMouseButton Button) const;
	bool IsMousePressed(EMouseButton Button) const;
	bool IsMouseUp(EMouseButton Button) const;

	FVector2 GetMousePosition() const;
	FVector2 GetMouseDelta() const;
	float GetMouseWheelDelta() const;

	void SetKey(uint32 Key, bool Value);
	void SetMousePosition(FVector2 Position);
	void SetMouseButton(EMouseButton Button, bool Value);
	void SetMouseWheel(float Delta);

	FInputManager(const FInputManager&) = delete;
	FInputManager& operator=(const FInputManager&) = delete;

	FInputManager(FInputManager&&) = delete;
	FInputManager& operator=(FInputManager&&) = delete;

private:
	FInputManager() = default;
	~FInputManager() = default;

	bool IsPrevKeyDown(uint32 Key) const;
	
	bool CurrentKeyStates[MAX_KEYS] = {};
	bool PreviousKeyStates[MAX_KEYS] = {};

	bool bCurrentMouseState[MAX_MOUSE_BUTTONS] = {};
	bool bPreviousMouseState[MAX_MOUSE_BUTTONS] = {};

	FVector2 CurrentMousePosition{};
	FVector2 PreviousMousePosition{};
	FVector2 MouseDelta{ 0.0f, 0.0f };

	float MouseWheelDelta = 0.0f;
	float AccmulatedWheelData = 0.0f;
};

enum class EMouseButton : uint8
{
	Left,
	Right,
	Middle,
	Back,
	Forward,
};

#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Math/FVector2.h"

enum class EMouseButton : uint8;

class FInputManager final
{
public:
	static constexpr int32 MAX_KEYS = 256;

	static FInputManager& Get()
	{
		static FInputManager Instance;
		return Instance;
	}
	
	void BeginFrame();

	[[nodiscard]] bool IsKeyDown(uint32 Key) const;
	[[nodiscard]] bool IsKeyJustPressed(uint32 Key) const;
	[[nodiscard]] bool IsKeyJustReleased(uint32 Key) const;

	[[nodiscard]] bool IsMouseDown(EMouseButton Button) const;
	[[nodiscard]] FVector2 GetMousePosition() const;
	[[nodiscard]] FVector2 GetMouseDelta() const;
	float GetMouseWheelDelta() const;

	void OnMouseMove(FVector2 Position);
	void OnMouseButtonDown(EMouseButton Button, FVector2 Position);
	void OnMouseButtonUp(EMouseButton Button, FVector2 Position);
	void OnMouseWheel(float Delta);

	FInputManager(const FInputManager&) = delete;
	FInputManager& operator=(const FInputManager&) = delete;

	FInputManager(FInputManager&&) = delete;
	FInputManager& operator=(FInputManager&&) = delete;

private:
	FInputManager() = default;
	~FInputManager() = default;

	[[nodiscard]] bool IsPrevKeyDown(uint32 Key) const;
	
	bool bIsRightButtonDown = false;
	bool CurrentKeyStates[MAX_KEYS] = {};
	bool PreviousKeyStates[MAX_KEYS] = {};

	FVector2 CurrentMousePosition{};
	FVector2 PreviousMousePosition{};
	bool bMouseLeftPressed = false;
	bool bMouseRightPressed = false;
	bool bMouseMiddlePressed = false;
	FVector2 MouseDelta{ 0.0f, 0.0f };

	float MouseWheelDelta = 0.0f;
	float AccmulatedWheelData = 0.0f;
};

enum class EMouseButton : uint8
{
	Left,
	Right,
	Middle,
};

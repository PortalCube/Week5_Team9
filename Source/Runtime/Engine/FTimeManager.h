#pragma once

#include <chrono>

class FTimeManager final
{
public:

	static FTimeManager& Get()
	{
		static FTimeManager Instance;
		return Instance;
	}

	void Initialize();

	void Resume() { bIsRunning = true; }
	void Pause() { bIsRunning = false; }

	float GetTime() const;
	float GetDeltaTime() const;

	void Update();
	void SetFPS(float InFPS) { FPS = InFPS; }

	FTimeManager(const FTimeManager&) = delete;
	FTimeManager& operator=(const FTimeManager&) = delete;

	FTimeManager(FTimeManager&&) = delete;
	FTimeManager&& operator=(FTimeManager&&) = delete;

private:

	using SteadyClock = std::chrono::steady_clock;
	using TimePoint = std::chrono::steady_clock::time_point;
	using Duration = std::chrono::duration<float>;

	FTimeManager();
	~FTimeManager() = default;

	TimePoint StartTime;
	TimePoint PrevTime;
	
	float FPS = 60.0f;
	float DeltaTime = 0.0f;

	float TempTime = 0.0f;
	float TempDeltaTime = 0.0f;

	bool bIsRunning = false;
};

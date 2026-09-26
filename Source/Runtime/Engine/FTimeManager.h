#pragma once

#include <chrono>

class FTimeManager
{
public:
	FTimeManager() = delete;

	static float GetTime();
	static float GetDeltaTime();

	static void Update();

private:

	using SteadyClock = std::chrono::steady_clock;
	using TimePoint = std::chrono::steady_clock::time_point;
	using Duration = std::chrono::duration<float>;

	inline static TimePoint StartTime = SteadyClock::now();
	inline static TimePoint PrevTime = StartTime;

	inline static float Time = 0.0f;
	inline static float DeltaTime = 0.0f;
};

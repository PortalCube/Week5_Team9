#include "FTimeManager.h"

float FTimeManager::GetTime()
{
	return Time;
}

float FTimeManager::GetDeltaTime()
{
	return DeltaTime;
}

void FTimeManager::Update()
{
	const TimePoint Clock = SteadyClock::now();
	Time = Duration(Clock - StartTime).count();
	DeltaTime = Duration(Clock - PrevTime).count();
	PrevTime = Clock;
}

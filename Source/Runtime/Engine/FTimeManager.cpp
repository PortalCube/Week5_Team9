#include <windows.h>
#include <chrono>
#include "FTimeManager.h"

float FTimeManager::GetTime() const
{
    //TimePoint Clock = SteadyClock::now();
    //return Duration(Clock - StartTime).count();
    return TempTime;
}

float FTimeManager::GetDeltaTime() const
{
    //TimePoint Clock = SteadyClock::now();
    //return Duration(Clock - PrevTime).count();
    return TempDeltaTime;
}

FTimeManager::FTimeManager()
{
    StartTime = SteadyClock::now();
    PrevTime = SteadyClock::now();
}

void FTimeManager::Update()
{
    float TargetTime = 1.0f / FPS;
    float DeltaTime;

    TimePoint Clock = SteadyClock::now();
    DeltaTime = Duration(Clock - PrevTime).count();

    //while (DeltaTime < TargetTime)
    //{
    //    _mm_pause();

    //    Clock = SteadyClock::now();
    //    DeltaTime = Duration(Clock - PrevTime).count();
    //}

    TempDeltaTime = DeltaTime;
    TempTime = Duration(Clock - StartTime).count();

    PrevTime = SteadyClock::now();
}
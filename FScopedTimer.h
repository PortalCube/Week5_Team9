#pragma once
#include <chrono>

class FScopedTimer
{
public:
    explicit FScopedTimer(double& OutTime)
        : OutTime(OutTime)
        , Start(std::chrono::high_resolution_clock::now())
    {
    }

    ~FScopedTimer()
    {
        const auto End = std::chrono::high_resolution_clock::now();
        OutTime = std::chrono::duration<double, std::milli>(End - Start).count();
    }

private:
    double& OutTime;
    std::chrono::high_resolution_clock::time_point Start;
};
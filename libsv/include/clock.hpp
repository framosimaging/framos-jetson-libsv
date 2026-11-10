#pragma once
#include <chrono>

class Clock
{
public:
    Clock() = delete;
    ~Clock() = delete;

    static void Start();
    static void Stop();
    static void Reset();
    static int Result();
    static int Count();
    static int Avg();

private:
    static std::chrono::steady_clock::time_point tp;

    static int sum;
    static int count;
};
#include "clock.hpp"

void Clock::Start()
{
    tp = std::chrono::steady_clock::now();
}

void Clock::Stop()
{
    sum += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - tp).count();
    count += 1;
}

void Clock::Reset()
{
    sum = 0;
    count = 0;
}

int Clock::Result()
{
    return sum;
}

int Clock::Count()
{
    return count;
}

int Clock::Avg()
{
    return sum/count;
}

std::chrono::steady_clock::time_point Clock::tp;
int Clock::sum = 0;
int Clock::count = 0;
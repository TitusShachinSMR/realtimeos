#include "SystemClock.h"

#include <chrono>

std::atomic<unsigned long> SystemClock::tickCount(0);
std::atomic<bool> SystemClock::running(false);
std::thread SystemClock::timerThread;

void SystemClock::start()
{
    running = true;
    timerThread = std::thread(tickThread);
}

void SystemClock::stop()
{
    running = false;

    if (timerThread.joinable())
        timerThread.join();
}

unsigned long SystemClock::getTick()
{
    return tickCount.load();
}

void SystemClock::tickThread()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        tickCount++;
    }
}
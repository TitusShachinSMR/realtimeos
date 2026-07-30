#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <atomic>
#include <thread>

class SystemClock
{
public:
    // Start the system tick
    static void start();

    // Stop the system tick
    static void stop();

    // Get current tick
    static unsigned long getTick();

private:
    static void tickThread();

    static std::atomic<unsigned long> tickCount;
    static std::atomic<bool> running;
    static std::thread timerThread;
};

#endif
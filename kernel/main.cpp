#include <iostream>

#include "Task.h"
#include "Scheduler.h"
#include "Mutex.h"

int main()
{
    std::cout << "==========================================\n";
    std::cout << " TinyRTOS - Complete Task Lifecycle Demo\n";
    std::cout << "==========================================\n";

    // Large boost interval so aging doesn't muddle the demo.
    Scheduler scheduler(100000);

    Mutex sharedBus(scheduler);

    // ------------------------------------------------------------
    // Alpha (priority 1) : READY -> RUNNING -> BLOCKED (delay)
    //                      -> READY -> RUNNING -> TERMINATED
    // ------------------------------------------------------------
    Task alpha(
        1,
        "Alpha",
        1,
        [](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                std::cout << "Alpha delays for 3 ticks\n";
                self.delay(3);
            }
            else
            {
                std::cout << "Alpha woke up from delay\n";
                self.terminate();
            }
        });

    // ------------------------------------------------------------
    // Beta (priority 5, highest) : READY -> RUNNING -> SUSPENDED
    //                     -> READY (resumed by Gamma)
    //                     -> RUNNING -> TERMINATED
    // ------------------------------------------------------------
    Task beta(
        2,
        "Beta",
        5,
        [](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                std::cout << "Beta suspends itself\n";
                self.suspend();
            }
            else
            {
                std::cout << "Beta running again after being resumed\n";
                self.terminate();
            }
        });

    // ------------------------------------------------------------
    // Gamma (priority 3) : READY -> RUNNING -> WAITING (mutex)
    //                      -> READY (woken by Delta's release)
    //                      -> RUNNING -> TERMINATED
    // It also resumes the SUSPENDED Beta task.
    // ------------------------------------------------------------
    Task gamma(
        3,
        "Gamma",
        3,
        [&](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                if (sharedBus.acquire(self))
                {
                    std::cout << "Gamma got the bus\n";
                }
                else
                {
                    std::cout << "Gamma blocks: WAITING for the bus\n";
                }
            }
            else
            {
                std::cout << "Gamma owns the bus after WAITING\n";
                sharedBus.release(self);

                std::cout << "Gamma resumes the suspended Beta\n";
                scheduler.resumeTask(&beta);

                self.terminate();
            }
        });

    // ------------------------------------------------------------
    // Delta (priority 4) : READY -> RUNNING -> BLOCKED (delay)
    //                      -> READY -> RUNNING -> TERMINATED
    // Holds the bus so Gamma goes WAITING.
    // ------------------------------------------------------------
    Task delta(
        4,
        "Delta",
        4,
        [&](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                std::cout << "Delta acquires the bus\n";
                sharedBus.acquire(self);
                self.delay(10);
            }
            else
            {
                std::cout << "Delta releases the bus\n";
                sharedBus.release(self);
                self.terminate();
            }
        });

    scheduler.createTask(&alpha);
    scheduler.createTask(&beta);
    scheduler.createTask(&gamma);
    scheduler.createTask(&delta);

    scheduler.start();

    return 0;
}
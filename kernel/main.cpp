#include <iostream>

#include "Task.h"
#include "Scheduler.h"
#include "Mutex.h"

int main()
{
    // Large boost interval so aging doesn't muddle the mutex demo.
    Scheduler scheduler(100000);

    Mutex sharedBus(scheduler);

    bool lowHoldsBus = false;

    Task low(
        1,
        "Low",
        1,
        [&](Task& self)
        {
            if (!lowHoldsBus)
            {
                if (sharedBus.acquire(self))
                {
                    lowHoldsBus = true;
                    std::cout << "Low acquired the bus\n";
                    self.delay(10);
                }
                else
                {
                    std::cout << "Low waiting for the bus\n";
                }
            }
            else
            {
                std::cout << "Low releasing the bus\n";
                sharedBus.release(self);
                lowHoldsBus = false;
            }
        });

    Task medium(
        2,
        "Medium",
        3,
        [](Task& self)
        {
            std::cout << "Medium running (does not need the bus)\n";
            self.delay(10);
        });

    Task high(
        3,
        "High",
        5,
        [&](Task& self)
        {
            if (sharedBus.acquire(self))
            {
                std::cout << "High using the bus\n";
                sharedBus.release(self);
                self.delay(10);
            }
            else
            {
                std::cout << "High waiting for the bus (blocks)\n";
            }
        });

    scheduler.createTask(&low);
    scheduler.createTask(&medium);
    scheduler.createTask(&high);

    scheduler.start();

    return 0;
}

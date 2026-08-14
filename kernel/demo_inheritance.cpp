#include <iostream>

#include "Task.h"
#include "Scheduler.h"
#include "Mutex.h"

int main()
{
    std::cout << "==========================================\n";
    std::cout << " Transitive Priority Inheritance Demo\n";
    std::cout << " A(5) waits on m1 held by B(3)\n";
    std::cout << " B(3) waits on m2 held by C(1)\n";
    std::cout << "==========================================\n\n";

    Scheduler scheduler(100000);

    Mutex m1(scheduler);
    Mutex m2(scheduler);

    // A (prio 5) depends on m1 -> B, which depends on m2 -> C.
    Task a(
        1,
        "A",
        5,
        [&](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                std::cout << "A delays first so B and C can build the chain\n";
                self.delay(20);
            }
            else if (self.getExecutionCount() == 2)
            {
                if (m1.acquire(self))
                {
                    std::cout << "A got m1\n";
                }
                else
                {
                    std::cout << "A blocks: WAITING on m1\n";
                }
            }
            else
            {
                m1.acquire(self);
                m1.release(self);
                std::cout << "A finished, releasing m1\n";
                self.terminate();
            }
        });

    // B (prio 3) holds m1, delays, then waits on m2 (held by C).
    Task b(
        2,
        "B",
        3,
        [&](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                if (m1.acquire(self))
                {
                    std::cout << "B acquired m1 (holds it)\n";
                }
                self.delay(10);
            }
            else if (self.getExecutionCount() == 2)
            {
                if (m2.acquire(self))
                {
                    std::cout << "B got m2\n";
                }
                else
                {
                    std::cout << "B blocks: WAITING on m2\n";
                }
            }
            else
            {
                m2.acquire(self);
                m2.release(self);
                m1.release(self);
                std::cout << "B released m1 -> A gets it\n";
                self.terminate();
            }
        });

    // C (prio 1) holds m2 across a long delay, then releases.
    Task c(
        3,
        "C",
        1,
        [&](Task& self)
        {
            if (self.getExecutionCount() == 1)
            {
                if (m2.acquire(self))
                {
                    std::cout << "C acquired m2 (holds it)\n";
                }
                self.delay(25);
            }
            else
            {
                std::cout << "C releases m2\n";
                m2.release(self);
                self.terminate();
            }
        });

    scheduler.createTask(&a);
    scheduler.createTask(&b);
    scheduler.createTask(&c);

    scheduler.start();

    return 0;
}
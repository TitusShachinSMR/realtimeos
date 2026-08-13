#ifndef MUTEX_H
#define MUTEX_H

#include <deque>

#include "Task.h"

class Scheduler;

class Mutex
{
public:
    Mutex(Scheduler& scheduler);

    // Returns true if the task got the mutex,
    // false if it had to wait (task is blocked).
    bool acquire(Task& task);
    void release(Task& task);

private:
    Task* highestPriorityWaiter() const;

    Scheduler& scheduler;
    Task* owner;
    std::deque<Task*> waitQueue;
};

#endif
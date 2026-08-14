#ifndef MUTEX_H
#define MUTEX_H

#include <deque>
#include <set>

#include "Task.h"

class Scheduler;

class Mutex
{
public:
    Mutex(Scheduler& scheduler);

    // Returns true if the task got the mutex,
    // false if it had to wait (task is blocked).
    //
    // The mutex is recursive: a task that already owns it may
    // acquire it again (a matching number of releases is needed
    // to actually free it).
    bool acquire(Task& task);
    void release(Task& task);

private:
    Task* highestPriorityWaiter() const;
    int highestWaiterPriority() const;

    // Transitive priority inheritance: raise every task that the
    // blocked task depends on (direct owner, then the owner's owner,
    // and so on) to at least the blocked task's priority.
    void inheritPriority(Mutex* root, Task& blockedTask);
    void raiseOwnerChain(Mutex* mutex,
                         int priority,
                         std::set<Mutex*>& visited);

    // Recompute a task's effective priority after it releases a mutex,
    // keeping any inheritance it still owes to remaining held mutexes.
    void restorePriority(Task& task);

    Scheduler& scheduler;
    Task* owner;
    int holdCount;
    std::deque<Task*> waitQueue;
};

#endif
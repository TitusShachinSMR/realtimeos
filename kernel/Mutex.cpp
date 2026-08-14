#include "Mutex.h"

#include <algorithm>
#include <iostream>

#include "Scheduler.h"

Mutex::Mutex(Scheduler& scheduler)
    : scheduler(scheduler),
      owner(nullptr),
      holdCount(0)
{
}

bool Mutex::acquire(Task& task)
{
    if (owner == nullptr)
    {
        owner = &task;
        holdCount = 1;
        task.setWaitingOn(nullptr);
        task.holdMutex(this);
        return true;
    }

    if (owner == &task)
    {
        // Recursive acquire, or re-acquire right after being handed
        // the mutex by a release (holdCount == 0 at that moment).
        holdCount = (holdCount == 0) ? 1 : holdCount + 1;
        task.setWaitingOn(nullptr);
        return true;
    }

    task.setState(TaskState::WAITING);
    task.setWaitingOn(this);
    waitQueue.push_back(&task);

    inheritPriority(this, task);

    return false;
}

void Mutex::release(Task& task)
{
    if (owner != &task)
    {
        return;
    }

    if (holdCount > 0)
    {
        holdCount--;

        if (holdCount > 0)
        {
            // Still held by recursive acquire() calls.
            return;
        }
    }

    // Full release.
    task.releaseMutex(this);
    restorePriority(task);

    if (!waitQueue.empty())
    {
        Task* next = highestPriorityWaiter();

        waitQueue.erase(
            std::find(waitQueue.begin(), waitQueue.end(), next));

        owner = next;
        holdCount = 0;

        next->setWaitingOn(nullptr);
        next->holdMutex(this);

        scheduler.wakeTask(next);
    }
    else
    {
        owner = nullptr;
        holdCount = 0;
    }
}

void Mutex::inheritPriority(Mutex* root, Task& blockedTask)
{
    std::set<Mutex*> visited;

    raiseOwnerChain(root, blockedTask.getPriority(), visited);
}

void Mutex::raiseOwnerChain(Mutex* mutex,
                            int priority,
                            std::set<Mutex*>& visited)
{
    if (visited.find(mutex) != visited.end())
    {
        return;
    }

    visited.insert(mutex);

    if (mutex->owner == nullptr)
    {
        return;
    }

    Task* ownerTask = mutex->owner;

    if (ownerTask->getPriority() < priority)
    {
        std::cout << "[Inherit] "
                  << ownerTask->getName()
                  << " boosted to priority "
                  << priority
                  << " (transitive inheritance)\n";

        scheduler.setTaskPriority(ownerTask, priority);
    }

    // If the owner is itself blocked on another mutex, keep
    // propagating so the whole dependency chain runs at a
    // priority high enough to hand the locks back up.
    Mutex* blockedOn = ownerTask->getWaitingOn();

    if (blockedOn != nullptr)
    {
        raiseOwnerChain(blockedOn, ownerTask->getPriority(), visited);
    }
}

void Mutex::restorePriority(Task& task)
{
    int target = task.getBasePriority();

    // The task may still hold other mutexes with waiters, so it must
    // keep any inheritance it still owes to them.
    for (Mutex* held : task.getHeldMutexes())
    {
        target = std::max(target, held->highestWaiterPriority());
    }

    if (task.getPriority() != target)
    {
        std::cout << "[Inherit] "
                  << task.getName()
                  << " priority restored to "
                  << target
                  << "\n";

        scheduler.setTaskPriority(&task, target);
    }
}

Task* Mutex::highestPriorityWaiter() const
{
    return *std::max_element(
        waitQueue.begin(),
        waitQueue.end(),
        [](Task* a, Task* b) {
            return a->getPriority() < b->getPriority();
        });
}

int Mutex::highestWaiterPriority() const
{
    int highest = -1;

    for (Task* waiter : waitQueue)
    {
        highest = std::max(highest, waiter->getPriority());
    }

    return highest;
}
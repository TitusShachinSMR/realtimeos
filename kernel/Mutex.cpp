#include "Mutex.h"

#include <algorithm>
#include <iostream>

#include "Scheduler.h"

Mutex::Mutex(Scheduler& scheduler)
    : scheduler(scheduler),
      owner(nullptr)
{
}

bool Mutex::acquire(Task& task)
{
    if (owner == nullptr)
    {
        owner = &task;
        return true;
    }

    if (owner == &task)
    {
        return true;
    }

    task.setState(TaskState::WAITING);
    waitQueue.push_back(&task);

    if (task.getPriority() > owner->getPriority())
    {
        std::cout << "[Inherit] "
                  << owner->getName()
                  << " boosted to priority "
                  << task.getPriority()
                  << " (so "
                  << task.getName()
                  << " is not delayed by it)\n";

        scheduler.setTaskPriority(owner, task.getPriority());
    }

    return false;
}

void Mutex::release(Task& task)
{
    if (owner != &task)
    {
        return;
    }

    if (task.getPriority() != task.getBasePriority())
    {
        std::cout << "[Inherit] "
                  << task.getName()
                  << " priority restored to "
                  << task.getBasePriority()
                  << "\n";

        scheduler.setTaskPriority(&task, task.getBasePriority());
    }

    if (!waitQueue.empty())
    {
        Task* next = highestPriorityWaiter();

        waitQueue.erase(
            std::find(waitQueue.begin(), waitQueue.end(), next));

        owner = next;

        scheduler.wakeTask(next);
    }
    else
    {
        owner = nullptr;
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
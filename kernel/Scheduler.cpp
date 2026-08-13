#include "Scheduler.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

#include "SystemClock.h"

Scheduler::Scheduler(int boostInterval)
    : boostCounter(0),
      boostInterval(boostInterval)
{
}

void Scheduler::createTask(Task* task)
{
    readyQueues[task->getPriority()].push_back(task);

    std::cout << "Task Created : "
              << task->getName()
              << " | Priority = "
              << task->getPriority()
              << std::endl;
}

int Scheduler::getHighestPriorityReadyQueue()
{
    int highest = -1;

    for (auto &entry : readyQueues)
    {
        if (!entry.second.empty())
        {
            highest = std::max(highest, entry.first);
        }
    }

    return highest;
}

int Scheduler::getLowestPriorityReadyQueue()
{
    int lowest = -1;

    for (auto &entry : readyQueues)
    {
        if (!entry.second.empty())
        {
            lowest = (lowest == -1) ? entry.first
                                    : std::min(lowest, entry.first);
        }
    }

    return lowest;
}

void Scheduler::boostLowPriorityTasks()
{
    int highest = getHighestPriorityReadyQueue();
    int lowest  = getLowestPriorityReadyQueue();

    if (lowest == -1 || lowest >= highest)
    {
        return;
    }

    auto &lowQueue = readyQueues[lowest];

    while (!lowQueue.empty())
    {
        Task* task = lowQueue.front();
        lowQueue.pop_front();

        readyQueues[lowest + 1].push_back(task);

        std::cout << "[Aging] "
                  << task->getName()
                  << " boosted from priority "
                  << lowest
                  << " to "
                  << lowest + 1
                  << "\n";
    }
}

void Scheduler::start()
{
    std::cout << "\n=====================================\n";
    std::cout << " TinyRTOS Priority Round Robin\n";
    std::cout << "=====================================\n\n";

    SystemClock::start();

    while (true)
    {
        // -----------------------------------
        // Wake delayed tasks
        // -----------------------------------
        for (auto it = delayQueue.begin(); it != delayQueue.end();)
        {
            Task* task = *it;

            if (SystemClock::getTick() >= task->getWakeupTick())
            {
                task->setState(TaskState::READY);

                std::cout
                    << "[Tick "
                    << SystemClock::getTick()
                    << "] "
                    << task->getName()
                    << " moved back to READY queue\n";

                readyQueues[task->getPriority()].push_back(task);

                it = delayQueue.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // -----------------------------------
        // Find highest priority queue
        // -----------------------------------
        int priority = getHighestPriorityReadyQueue();

        if (priority == -1)
        {
            std::cout << "No READY tasks.\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            continue;
        }

        // -----------------------------------
        // Get next task from that priority
        // -----------------------------------
        Task* currentTask = readyQueues[priority].front();

        readyQueues[priority].pop_front();

        // -----------------------------------
        // Run task
        // -----------------------------------
        std::cout << "---------------------------------\n";
        std::cout << "Tick     : "
                  << SystemClock::getTick()
                  << "\n";

        std::cout << "Running  : "
                  << currentTask->getName()
                  << "\n";

        std::cout << "Priority : "
                  << currentTask->getPriority()
                  << "\n";

        std::cout << "Run No.  : "
                  << currentTask->getExecutionCount() + 1
                  << "\n";

        std::cout << "---------------------------------\n";

        currentTask->run();

        // -----------------------------------
        // Put task into appropriate queue
        // -----------------------------------
        if (currentTask->isBlocked())
        {
            delayQueue.push_back(currentTask);

            std::cout
                << currentTask->getName()
                << " moved to DELAY queue\n";
        }
        else
        {
            readyQueues[currentTask->getPriority()].push_back(currentTask);
        }

        boostCounter++;

        if (boostCounter >= boostInterval)
        {
            boostLowPriorityTasks();
            boostCounter = 0;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500));
    }

    SystemClock::stop();
}
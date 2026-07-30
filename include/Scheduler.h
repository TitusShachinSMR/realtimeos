#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <vector>

#include "Task.h"

class Scheduler
{
public:
    void createTask(Task* task);
    void start();

private:

    std::unordered_map<int, std::deque<Task*>> readyQueues;
    std::deque<Task*> delayQueue;
    int getHighestPriorityReadyQueue();
};

#endif
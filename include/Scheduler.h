#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <deque>
#include <queue>
#include <unordered_map>
#include <vector>

#include "Task.h"

class Scheduler
{
public:
    Scheduler(int boostInterval = 1);

    void createTask(Task* task);
    void start();

private:
    std::unordered_map<int, std::deque<Task*>> readyQueues;
    std::deque<Task*> delayQueue;

    int getHighestPriorityReadyQueue();
    int getLowestPriorityReadyQueue();
    void boostLowPriorityTasks();

    int boostCounter;
    int boostInterval;
};

#endif
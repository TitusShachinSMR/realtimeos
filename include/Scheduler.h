#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <deque>
#include <unordered_map>

#include "Task.h"

class Scheduler
{
public:
    Scheduler(int boostInterval = 1);

    void createTask(Task* task);
    void start();

    // Used by kernel objects (e.g. Mutex) to move a task
    // between ready queues when its priority changes.
    void setTaskPriority(Task* task, int newPriority);
    void wakeTask(Task* task);

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
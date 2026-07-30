#ifndef TASK_H
#define TASK_H

#include <functional>
#include <string>

enum class TaskState
{
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    TERMINATED
};

class Task
{
public:
    Task(int id,
         const std::string& name,
         int priority,
         std::function<void(Task&)> function);

    void run();

    int getId() const;
    std::string getName() const;
    int getPriority() const;
    TaskState getState() const;

    void setState(TaskState newState);
    bool isReady() const;

    int getExecutionCount() const;
void delay(unsigned long ticks);

unsigned long getWakeupTick() const;

bool isBlocked() const;

private:
    int id;
    std::string name;
    int priority;
    TaskState state;
    int executionCount;
    std::function<void(Task&)> function;
    unsigned long wakeupTick;
};

struct TaskCompare
{
    bool operator()(Task* a, Task* b) const
    {
        return a->getPriority() < b->getPriority();
    }
};

#endif
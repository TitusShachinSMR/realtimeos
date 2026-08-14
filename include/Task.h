#ifndef TASK_H
#define TASK_H

#include <functional>
#include <string>
#include <vector>

class Mutex;

enum class TaskState
{
    READY,
    RUNNING,
    BLOCKED,
    WAITING,
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
    int getBasePriority() const;
    TaskState getState() const;

    void setPriority(int newPriority);
    void setState(TaskState newState);
    bool isReady() const;
    bool isWaiting() const;

    int getExecutionCount() const;
    void delay(unsigned long ticks);

    unsigned long getWakeupTick() const;

    bool isBlocked() const;

    // Lifecycle control
    void suspend();
    void resume();
    void terminate();

    bool isSuspended() const;
    bool isTerminated() const;

    // Priority-inheritance bookkeeping (used by Mutex).
    void setWaitingOn(Mutex* mutex);
    Mutex* getWaitingOn() const;

    void holdMutex(Mutex* mutex);
    void releaseMutex(Mutex* mutex);
    const std::vector<Mutex*>& getHeldMutexes() const;

private:
    int id;
    std::string name;
    int priority;
    int basePriority;
    TaskState state;
    int executionCount;
    std::function<void(Task&)> function;
    unsigned long wakeupTick;
    Mutex* waitingOn;
    std::vector<Mutex*> heldMutexes;
};

#endif
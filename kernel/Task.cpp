#include "Task.h"
#include "SystemClock.h"
#include <algorithm>
#include <iostream>
Task::Task(int id,
           const std::string& name,
           int priority,
           std::function<void(Task&)> function)
    : id(id),
      name(name),
      priority(priority),
      basePriority(priority),
      state(TaskState::READY),
      executionCount(0),
      function(function),
      wakeupTick(0),
      waitingOn(nullptr),
      heldMutexes()
{
}

void Task::run()
{
    executionCount++;

    state = TaskState::RUNNING;

    std::cout << "[State] "
              << name
              << " -> RUNNING (run no. "
              << executionCount
              << ")\n";

    function(*this);

    // If the task didn't block / suspend / terminate itself,
    // make it READY again.
    if (state == TaskState::RUNNING)
    {
        state = TaskState::READY;

        std::cout << "[State] "
                  << name
                  << " -> READY (yields CPU)\n";
    }
}
int Task::getId() const
{
    return id;
}

std::string Task::getName() const
{
    return name;
}

int Task::getPriority() const
{
    return priority;
}

int Task::getBasePriority() const
{
    return basePriority;
}

void Task::setPriority(int newPriority)
{
    priority = newPriority;
}

TaskState Task::getState() const
{
    return state;
}

void Task::setState(TaskState newState)
{
    state = newState;
}

bool Task::isReady() const
{
    return state == TaskState::READY;
}

bool Task::isWaiting() const
{
    return state == TaskState::WAITING;
}

int Task::getExecutionCount() const
{
    return executionCount;
}

void Task::delay(unsigned long ticks)
{
    std::cout << "[DEBUG] "
              << name
              << " blocked until tick "
              << SystemClock::getTick() + ticks
              << std::endl;

    state = TaskState::BLOCKED;
    wakeupTick = SystemClock::getTick() + ticks;
}

bool Task::isBlocked() const
{
    return state == TaskState::BLOCKED;
}

void Task::suspend()
{
    state = TaskState::SUSPENDED;

    std::cout << "[State] "
              << name
              << " -> SUSPENDED (paused, must be resumed)\n";
}

void Task::resume()
{
    state = TaskState::READY;

    std::cout << "[State] "
              << name
              << " -> READY (resumed)\n";
}

void Task::terminate()
{
    state = TaskState::TERMINATED;

    std::cout << "[State] "
              << name
              << " -> TERMINATED (never scheduled again)\n";
}

bool Task::isSuspended() const
{
    return state == TaskState::SUSPENDED;
}

bool Task::isTerminated() const
{
    return state == TaskState::TERMINATED;
}

void Task::setWaitingOn(Mutex* mutex)
{
    waitingOn = mutex;
}

Mutex* Task::getWaitingOn() const
{
    return waitingOn;
}

void Task::holdMutex(Mutex* mutex)
{
    heldMutexes.push_back(mutex);
}

void Task::releaseMutex(Mutex* mutex)
{
    heldMutexes.erase(
        std::remove(heldMutexes.begin(), heldMutexes.end(), mutex),
        heldMutexes.end());
}

const std::vector<Mutex*>& Task::getHeldMutexes() const
{
    return heldMutexes;
}

unsigned long Task::getWakeupTick() const
{
    return wakeupTick;
}
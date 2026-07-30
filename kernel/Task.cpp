#include "Task.h"
#include "SystemClock.h"
#include <iostream>
Task::Task(int id,
           const std::string& name,
           int priority,
           std::function<void(Task&)> function)
    : id(id),
      name(name),
      priority(priority),
      state(TaskState::READY),
      executionCount(0),
      wakeupTick(0),
      function(function)
{
}

void Task::run()
{
    executionCount++;

    state = TaskState::RUNNING;

    function(*this);

    // If the task didn't block itself,
    // make it READY again.
    if (state == TaskState::RUNNING)
    {
        state = TaskState::READY;
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

unsigned long Task::getWakeupTick() const
{
    return wakeupTick;
}
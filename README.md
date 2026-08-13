# TinyRTOS

A miniature Real-Time Operating System (RTOS) kernel implemented in **Modern C++** to understand how commercial RTOSes such as **FreeRTOS**, **ThreadX**, and **Zephyr** organize task scheduling and system timing.

> **Note:** This is an educational RTOS simulator running on a desktop PC. It simulates RTOS concepts using C++ threads and timers; it is not intended for deployment on embedded hardware.

---

# Features

* Task Creation
* Task States

  * READY
  * RUNNING
  * BLOCKED
  * WAITING
  * SUSPENDED
  * TERMINATED
* System Tick Generator
* Delay API (`delay(ticks)`)
* Delay Queue
* Strict Priority Scheduling
* Priority Round Robin Scheduling
* **Priority Aging** (prevents low-priority starvation)
* **Mutex with Priority Inheritance** (fixes priority inversion)
* Simulated Context Switching
* Modular Kernel Architecture

---

# Architecture

```
                   +--------------------+
                   |    main.cpp        |
                   +---------+----------+
                             |
                             v
                  Creates Tasks, Scheduler & Mutex
                             |
                             v
                  +----------------------+
                  |     Scheduler        |
                  +----------+-----------+
                             |
        +--------------------+-------------------+
        |                                        |
        v                                        v
+----------------------+            +----------------------+
|   Ready Queues       |            |    Delay Queue       |
| (One Queue/Priority) |            | Sleeping Tasks       |
+----------+-----------+            +----------+-----------+
           |                                   |
           |                                   |
           v                                   |
      Select Highest                          |
      Priority Task                           |
           |                                  |
           v                                  |
      Task::run()                             |
           |                                  |
           | delay(ticks)                     |
           +--------------------------------->|
           |                                  |
           | Mutex::acquire()                 |
           +------------+                     |
                        |                     |
                        v                     |
                 Mutex Wait Queue             |
                 (blocked on mutex)           |
                        |                     |
              Mutex::release()                |
                        |                     |
                        v                     |
              Wake highest-priority waiter    |
                        |                     |
                        +-------------------->|
                                             |
                                  Wakeup Tick Reached
                                             |
                                             v
                                     Back to READY Queue
```

---

# Project Structure

```
TinyRTOS/
│
├── include/
│   ├── Task.h
│   ├── Scheduler.h
│   ├── Mutex.h
│   └── SystemClock.h
│
├── kernel/
│   ├── main.cpp
│   ├── Task.cpp
│   ├── Scheduler.cpp
│   ├── Mutex.cpp
│   └── SystemClock.cpp
│
├── README.md
│
└── CMakeLists.txt
```

---

# Components

## 1. Task

Represents a single RTOS task.

Each task stores:

* Task ID
* Name
* Priority (effective / current priority)
* Base Priority (original priority, restored after mutex release)
* Current State
* Wake-up Tick
* Execution Count
* Function Pointer

Example:

```cpp
Task low(
    1,
    "Low",
    1,
    [](Task& self)
    {
        std::cout << "Low running\n";
    }
);
```

---

## 2. Scheduler

The scheduler is the heart of the kernel.

Responsibilities:

* Maintain Ready Queues
* Maintain Delay Queue
* Select highest-priority task
* Perform Round Robin among equal priorities
* Wake delayed tasks
* Boost low-priority tasks (aging)
* Dispatch tasks
* Move tasks between queues when priority changes (used by Mutex)

---

## 3. System Clock

A background timer thread simulates the RTOS system tick.

Every 100 ms:

```
Tick++
```

Tasks use the tick count for delays.

---

## 4. Mutex (with Priority Inheritance)

A mutual-exclusion lock that serializes access to a shared resource.

* `acquire(task)` — if the mutex is free the task takes it. If it is held by
  another task, the task is blocked (`WAITING` state) and added to the mutex
  wait queue.
* `release(task)` — releases the mutex, hands it to the **highest-priority
  waiter**, and wakes that waiter.

If a high-priority task blocks on a mutex held by a low-priority task, the
kernel **temporarily raises the holder's priority** to the waiter's priority
(priority inheritance). This stops medium-priority tasks from delaying the
hand-off — the classic fix for **priority inversion**.

---

# Scheduling Algorithm

The scheduler runs a continuous loop:

```
while(true)

    Wake delayed tasks (tick reached -> back to READY queue)

    Find highest priority READY queue

    Pop front task

    Execute task

    if task delayed            -> Move to Delay Queue
    else if task waiting       -> Leave it in the Mutex wait queue
    else                       -> Push to back of same priority queue

    Every boostInterval runs:
        Boost lowest-priority queue by one level (aging)
```

## Priority Round Robin

Tasks are grouped by priority. The scheduler always picks the **highest
non-empty** priority queue, then runs tasks within that queue in round-robin
order. If only one highest-priority task exists, it keeps running until it
blocks or delays.

## Priority Aging

Strict priority scheduling can **starve** low-priority tasks — the highest
priority task always runs, so low-priority tasks never get CPU time.

TinyRTOS solves this with aging: after each task run, a counter is
incremented. When the counter reaches `boostInterval`, the lowest-priority
ready queue is moved up **one priority level** (see `boostLowPriorityTasks`).
This lets low-priority jobs cyclically climb the priority ladder and get their
turn, while high-priority tasks still run most often.

## Priority Inversion & Inheritance

The `main.cpp` demo shows the classic priority-inversion scenario:

1. **Low** (priority 1) acquires the shared bus and holds it across a delay.
2. **High** (priority 5) wakes up and tries to acquire the bus -> it is held
   by Low, so High blocks.
3. Without inheritance, **Medium** (priority 3) would keep running and delay
   Low -> High would wait a long time.
4. With **priority inheritance**, the kernel boosts Low to priority 5:
   `[Inherit] Low boosted to priority 5`.
5. Low now runs before Medium, finishes its work, releases the bus, and hands
   it to High: `[Inherit] Low priority restored to 1`.
6. High acquires the bus and continues immediately.

---

# Sample Output

```
Task Created : Low | Priority = 1
Task Created : Medium | Priority = 3
Task Created : High | Priority = 5

=====================================
 TinyRTOS Priority Round Robin
=====================================

---------------------------------
Tick     : 0
Running  : High
Priority : 5
Run No.  : 1
---------------------------------
High using the bus
[DEBUG] High blocked until tick 10
High moved to DELAY queue
---------------------------------
Tick     : 4
Running  : Medium
Priority : 3
Run No.  : 1
---------------------------------
Medium running (does not need the bus)
[DEBUG] Medium blocked until tick 14
Medium moved to DELAY queue
---------------------------------
Tick     : 9
Running  : Low
Priority : 1
Run No.  : 1
---------------------------------
Low acquired the bus
[DEBUG] Low blocked until tick 19
Low moved to DELAY queue
[Tick 13] High moved back to READY queue
---------------------------------
Tick     : 13
Running  : High
Priority : 5
Run No.  : 2
---------------------------------
[Inherit] Low boosted to priority 5 (so High is not delayed by it)
High waiting for the bus (blocks)
---------------------------------
Tick     : 23
Running  : Low
Priority : 5
Run No.  : 2
---------------------------------
Low releasing the bus
[Inherit] Low priority restored to 1
---------------------------------
Tick     : 27
Running  : High
Priority : 5
Run No.  : 3
---------------------------------
High using the bus
```

Notice how **Low runs at priority 5** (boosted by inheritance) so it can
release the bus before Medium gets in the way.

---

# Build & Run

## Requirements

* C++17 compatible compiler (GCC, Clang, or MSVC)
* Any terminal (or VS Code's integrated terminal)

## Windows (MinGW)

Compile:

```bash
g++ -std=c++17 ^
kernel/main.cpp ^
kernel/Task.cpp ^
kernel/Scheduler.cpp ^
kernel/Mutex.cpp ^
kernel/SystemClock.cpp ^
-Iinclude ^
-o TinyRTOS.exe
```

Run:

```bash
TinyRTOS.exe
```

## Linux / macOS

Compile:

```bash
g++ -std=c++17 \
kernel/main.cpp \
kernel/Task.cpp \
kernel/Scheduler.cpp \
kernel/Mutex.cpp \
kernel/SystemClock.cpp \
-Iinclude \
-pthread \
-o TinyRTOS
```

Run:

```bash
./TinyRTOS
```

## VS Code

1. Open the folder: `File > Open Folder` -> select the project folder.
2. Open the integrated terminal: `` Ctrl + ` ``.
3. Run the compile command above, then `.\TinyRTOS.exe` (Windows) or
   `./TinyRTOS` (Linux/macOS).
4. The program runs forever, so stop it with `Ctrl + C`.

> The scheduler sleeps ~500 ms between task runs so you can watch the
> scheduling decisions in real time.

---

# Tuning the Demo

* `Scheduler scheduler(boostInterval)` — how often aging boosts low-priority
  tasks. A small value (e.g. 3) makes low-priority tasks get served quickly.
  A large value (e.g. 100000) effectively disables aging, which the mutex demo
  uses to keep output focused.
* Change task priorities and delays in `kernel/main.cpp` to see different
  scheduling patterns.

---

# Future Improvements

Potential extensions include:

* Binary Semaphore
* Counting Semaphore
* Message Queue
* Software Timers
* Event Flags
* EDF Scheduling
* Static Memory Pool
* Embedded Hardware Port (STM32 / ESP32)

---

# Learning Outcomes

This project helps build an understanding of:

* Operating System Scheduling
* RTOS Design
* Task Management
* Priority Scheduling
* Round Robin Scheduling
* Priority Aging (preventing starvation)
* Priority Inversion & Priority Inheritance
* Mutexes and Critical Sections
* Delay Management
* System Tick Generation
* Concurrent Programming in Modern C++
* RTOS Kernel Architecture

---

# Author

**Titus Shachin**

Built as a learning project to understand RTOS internals and operating system scheduling concepts using Modern C++.
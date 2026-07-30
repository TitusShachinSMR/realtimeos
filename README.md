# TinyRTOS

A miniature Real-Time Operating System (RTOS) kernel implemented in **Modern C++** to understand how commercial RTOSes such as **FreeRTOS**, **ThreadX**, and **Zephyr** organize task scheduling and system timing.

> **Note:** This is an educational RTOS simulator running on a desktop PC. It simulates RTOS concepts using C++ threads and timers; it is not intended for deployment on embedded hardware.

---

# Features

* Task Creation
* Task Control Block (Task object)
* Task States

  * READY
  * RUNNING
  * BLOCKED
  * SUSPENDED
  * TERMINATED
* System Tick Generator
* Delay API (`delay(ticks)`)
* Delay Queue
* Strict Priority Scheduling
* Priority Round Robin Scheduling
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
                  Creates Tasks & Scheduler
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
│   └── SystemClock.h
│
├── kernel/
│   ├── main.cpp
│   ├── Task.cpp
│   ├── Scheduler.cpp
│   └── SystemClock.cpp
│
├── drivers/
│
├── examples/
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
* Priority
* Current State
* Wake-up Tick
* Execution Count
* Function Pointer

Example:

```cpp
Task led(
    1,
    "LED",
    2,
    [](Task& self)
    {
        std::cout << "Blinking LED\n";
        self.delay(5);
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
* Dispatch tasks

Scheduling policy:

1. Choose highest non-empty priority queue.
2. Execute the first task.
3. Move task to:

   * Delay Queue (if blocked)
   * Back to Ready Queue (otherwise)

---

## 3. System Clock

A background timer thread simulates the RTOS system tick.

Every 100 ms:

```
Tick++
```

Tasks use the tick count for delays.

---

## Scheduling Algorithm

```
while(true)

    Wake delayed tasks

    Find highest priority READY queue

    Pop front task

    Execute task

    if task delayed

        Move to Delay Queue

    else

        Push to back of same priority queue
```

---

# Delay Queue

Instead of keeping blocked tasks inside the Ready Queue:

```
READY

LED
UART
Sensor
Motor
```

they are moved into a separate queue.

```
READY QUEUES

Priority 5
Sensor

Priority 3
Motor

Priority 2
LED

Priority 1
UART

-------------------------

DELAY QUEUE

Logger
Communication
Network
```

When the wake-up tick arrives:

```
Delay Queue

↓

READY Queue

↓

Scheduler
```

This organization is similar to commercial RTOS kernels.

---

# Priority Round Robin

Tasks are grouped by priority.

Example:

```
Priority 5

Sensor A
Sensor B

Priority 3

Motor

Priority 2

LED

Priority 1

UART
```

Execution order:

```
Sensor A

↓

Sensor B

↓

Sensor A

↓

Sensor B
```

If only one highest-priority task exists, it continues executing until it blocks or delays.

---

# Sample Output

```
Task Created : LED | Priority = 2
Task Created : UART | Priority = 1
Task Created : Sensor | Priority = 5
Task Created : Motor | Priority = 3

=====================================
 TinyRTOS Priority Round Robin
=====================================

Tick : 0
Running : Sensor

Reading Sensor

Tick : 5

Running : Sensor
```

---

# Build Instructions

## Requirements

* C++17 compatible compiler (GCC, Clang, or MSVC)

---

## Windows (MinGW)

Compile:

```bash
g++ -std=c++17 ^
kernel/main.cpp ^
kernel/Task.cpp ^
kernel/Scheduler.cpp ^
kernel/SystemClock.cpp ^
-Iinclude ^
-o TinyRTOS.exe
```

Run:

```bash
TinyRTOS.exe
```

---

## Linux / macOS

Compile:

```bash
g++ -std=c++17 \
kernel/main.cpp \
kernel/Task.cpp \
kernel/Scheduler.cpp \
kernel/SystemClock.cpp \
-Iinclude \
-pthread \
-o TinyRTOS
```

Run:

```bash
./TinyRTOS
```

---

# Future Improvements

Potential extensions include:

* Binary Semaphore
* Counting Semaphore
* Mutex
* Message Queue
* Software Timers
* Event Flags
* Priority Inheritance
* EDF Scheduling
* Static Memory Pool
* Embedded Hardware Port (STM32 / ESP32)

---

# Learning Outcomes

This project helped build an understanding of:

* Operating System Scheduling
* RTOS Design
* Task Management
* Priority Scheduling
* Round Robin Scheduling
* Delay Management
* System Tick Generation
* Concurrent Programming in Modern C++
* RTOS Kernel Architecture

---

# Author

**Titus Shachin**

Built as a learning project to understand RTOS internals and operating system scheduling concepts using Modern C++.

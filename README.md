# TinyRTOS

A miniature Real-Time Operating System (RTOS) kernel implemented in **Modern C++** to understand how commercial RTOSes such as **FreeRTOS**, **ThreadX**, and **Zephyr** organize task scheduling and system timing.

> **Note:** This is an educational RTOS simulator running on a desktop PC. It simulates RTOS concepts using C++ threads and timers; it is not intended for deployment on embedded hardware.

---

# Quick Start

The project ships **two runnable demos**:

| Demo | File | What it shows |
|------|------|---------------|
| **Lifecycle demo** | `kernel/main.cpp` | The complete task lifecycle: READY, RUNNING, BLOCKED, WAITING, SUSPENDED, TERMINATED |
| **Inheritance demo** | `kernel/demo_inheritance.cpp` | Priority inversion + **transitive** priority inheritance through a 3-task mutex chain |

## Requirements

* A C++17 compiler (GCC, Clang, or MSVC) and any terminal (or VS Code's integrated terminal)
* `-pthread` on Linux/macOS (POSIX threads)

## Windows (MinGW)

**Lifecycle demo:**

```bash
g++ -std=c++17 ^
kernel/main.cpp ^
kernel/Task.cpp ^
kernel/Scheduler.cpp ^
kernel/Mutex.cpp ^
kernel/SystemClock.cpp ^
-Iinclude ^
-pthread ^
-o TinyRTOS.exe

TinyRTOS.exe
```

**Inheritance demo:**

```bash
g++ -std=c++17 ^
kernel/demo_inheritance.cpp ^
kernel/Task.cpp ^
kernel/Scheduler.cpp ^
kernel/Mutex.cpp ^
kernel/SystemClock.cpp ^
-Iinclude ^
-pthread ^
-o TinyRTOS_Inheritance.exe

TinyRTOS_Inheritance.exe
```

## Linux / macOS

**Lifecycle demo:**

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

./TinyRTOS
```

**Inheritance demo:**

```bash
g++ -std=c++17 \
kernel/demo_inheritance.cpp \
kernel/Task.cpp \
kernel/Scheduler.cpp \
kernel/Mutex.cpp \
kernel/SystemClock.cpp \
-Iinclude \
-pthread \
-o TinyRTOS_Inheritance

./TinyRTOS_Inheritance
```

## CMake (optional)

Both demos are registered in `CMakeLists.txt`:

```bash
cmake -S . -B build
cmake --build build
```

This produces `TinyRTOS` and `TinyRTOS_Inheritance` executables.

> The scheduler sleeps ~500 ms between task runs so you can watch scheduling
> decisions in real time. Both demos exit by themselves once every task is
> `TERMINATED`.

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

* System Tick Generator (background timer thread)
* Delay API (`delay(ticks)`) with a Delay Queue
* Strict Priority Scheduling
* Priority Round Robin Scheduling (equal priorities share the CPU)
* **Priority Aging** — boosts low-priority tasks to prevent starvation
* **Mutex with Priority Inheritance** — fixes priority inversion
  * **Recursive mutex** — a task can acquire the same mutex multiple times
  * **Transitive inheritance** — the boost propagates through nested mutex chains
* Task lifecycle control: `suspend()` / `resume()` / `terminate()`
* Clean kernel shutdown when all tasks terminate
* Simulated Context Switching
* Modular Kernel Architecture

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
│   ├── main.cpp             <- Lifecycle demo
│   ├── demo_inheritance.cpp <- Transitive inheritance demo
│   ├── Task.cpp
│   ├── Scheduler.cpp
│   ├── Mutex.cpp
│   └── SystemClock.cpp
│
├── README.md
└── CMakeLists.txt
```

---

# Architecture

```
                    +--------------------+
                    |     main.cpp       |
                    +---------+----------+
                              |
                              v
                  Creates Tasks, Scheduler & Mutex(es)
                              |
                              v
                   +----------------------+
                   |      Scheduler       |
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

# Components

## 1. Task

Represents a single RTOS task. Each task stores:

* Task ID
* Name
* Priority (effective / current priority)
* Base Priority (original priority, restored after mutex release)
* Current State (`TaskState`)
* Wake-up Tick (used by `delay()`)
* Execution Count (how many times it has run)
* Function Pointer (the task body)
* Priority-inheritance bookkeeping: `waitingOn` (which mutex it is blocked on) and `heldMutexes` (mutexes it currently owns)

```cpp
Task worker(
    1,
    "Worker",
    2,
    [](Task& self)
    {
        std::cout << "Worker running\n";
        self.delay(10);     // BLOCKED for 10 ticks
    }
);
```

## 2. Scheduler

The scheduler is the heart of the kernel. Responsibilities:

* Maintain Ready Queues (one `std::deque` per priority level)
* Maintain the Delay Queue
* Select the highest-priority READY task
* Perform Round Robin among equal priorities
* Wake delayed tasks when their tick is reached
* Boost low-priority queues (aging)
* Dispatch tasks and route them to the correct queue after each run
* Move tasks between queues when priority changes (used by the Mutex)
* Resume `SUSPENDED` tasks via `resumeTask()`
* Shut down cleanly when every task is `TERMINATED`

## 3. System Clock

A background timer thread simulates the RTOS system tick:

```
every 100 ms:  Tick++
```

`SystemClock::getTick()` is atomic and drives the delay queue.

## 4. Mutex (with Priority Inheritance)

A mutual-exclusion lock that serializes access to a shared resource.

* `acquire(task)`
  * Mutex free -> the task takes it (`READY`/`RUNNING` continues).
  * Already owned by the *same* task -> **recursive**: a hold-count is incremented.
  * Held by another task -> the task is blocked (`WAITING`), added to the wait
    queue, and the holder is boosted via **priority inheritance**.
* `release(task)` — decrements the hold-count; only when it reaches zero does
  the mutex actually free. It then hands ownership to the **highest-priority
  waiter** and wakes it.

---

# How It Works — Step by Step

1. `main()` creates a `Scheduler` and the `Task`s, then calls `scheduler.start()`.
2. `SystemClock::start()` launches a background thread that increments the tick every 100 ms.
3. The scheduler enters its main loop:

   ```
   while (true)

       1. If every created task is TERMINATED -> exit the loop.

       2. Wake delayed tasks: for each task in the Delay Queue whose
          wakeup tick has been reached, move it BLOCKED -> READY and
          push it back onto its ready queue.

       3. Find the highest-priority non-empty ready queue.

       4. If there is none -> the kernel idles for 100 ms and repeats.

       5. Pop the front task and run it:
             Task::run() sets the state to RUNNING and invokes the body.

       6. Route the task based on its new state:
             TERMINATED -> remove it forever
             SUSPENDED  -> hold it out of every queue until resumed
             BLOCKED    -> push it onto the Delay Queue
             WAITING    -> leave it in the Mutex wait queue
             otherwise  -> push it to the back of its ready queue (yield)

       7. Every `boostInterval` runs, boost the lowest-priority queue (aging).

       8. Sleep ~500 ms so you can watch each decision, then repeat.
   ```

4. When the last task calls `terminate()`, the loop detects that every task is
   `TERMINATED`, prints `All tasks TERMINATED - kernel done`, stops the clock,
   and returns.

---

# Task Lifecycle

Every `TaskState` is exercised by the lifecycle demo (`kernel/main.cpp`):

```
NEW (constructor)
   -> READY          created, waiting for CPU
   -> RUNNING        the scheduler dispatched it
        -> BLOCKED   self.delay(ticks)  -> woken by the tick -> READY
        -> WAITING   Mutex::acquire() blocked -> released -> READY
        -> SUSPENDED self.suspend() -> Scheduler::resumeTask() -> READY
   -> TERMINATED     self.terminate(), never scheduled again
```

### Lifecycle API

```cpp
task.suspend();                 // RUNNING -> SUSPENDED, removed from all queues
scheduler.resumeTask(&task);    // SUSPENDED -> READY, back on the ready queue
task.terminate();               // -> TERMINATED, never scheduled again
task.delay(ticks);              // RUNNING -> BLOCKED, wakes up after N ticks
task.isReady();                 // state checks: isBlocked(), isWaiting(),
task.isSuspended();             //              isTerminated(), ...
```

A task controls its own `suspend()` / `terminate()` / `delay()` from inside its
function. `resume()` must go through the scheduler because the scheduler owns
the ready queues.

---

# Scheduling Algorithm

## Strict Priority + Round Robin

Tasks are grouped by priority (higher number = higher priority). The scheduler
always picks the **highest non-empty** ready queue, then runs tasks within that
queue in round-robin order. If only one highest-priority task exists, it keeps
running until it blocks, delays, suspends, or terminates.

## Priority Aging (anti-starvation / "boosting")

Strict priority scheduling can **starve** low-priority tasks — the highest
priority task always runs, so low-priority tasks never get CPU time.

TinyRTOS solves this with **aging**: after each task run a counter is
incremented. When it reaches `boostInterval`, the **lowest-priority ready
queue is moved up one priority level** (`boostLowPriorityTasks`):

```
[Aging] TaskName boosted from priority 1 to 2
```

Low-priority jobs therefore cyclically climb the priority ladder and get their
turn, while high-priority tasks still run most often. Tune it with
`Scheduler scheduler(boostInterval)`:
* a small value (e.g. `3`) — low-priority tasks get served very quickly;
* a large value (e.g. `100000`) — aging is effectively disabled, which keeps
  the mutex demos' output focused.

---

# Priority Inversion & Inheritance

## The Problem: Priority Inversion

If a **high-priority** task (H) blocks on a mutex held by a **low-priority**
task (L), and a **medium-priority** task (M) is ready, M will run before L
(since M > L). M delays L, L delays H — so H, the highest priority task in the
system, is effectively held up by M. This is **priority inversion**.

## The Fix: Priority Inheritance

When H blocks on L's mutex, the kernel **temporarily raises L's priority to
H's priority**. Now L beats M, finishes its critical section, releases the
mutex, and hands it to H immediately. When L releases, its priority is restored
to its base priority.

```
[Inherit] Low boosted to priority 5 (transitive inheritance)
...
[Inherit] Low priority restored to 1
```

## Transitive Inheritance (the chain case)

Inheritance also propagates **through a chain of mutexes**. If:

* A (prio 5) waits on `m1` held by B (prio 3), and
* B waits on `m2` held by C (prio 1),

then when A blocks, the kernel boosts **B** to 5 **and then C to 5**, so the
whole dependency chain runs at a priority high enough to hand the locks back
up. This is shown by the `demo_inheritance.cpp` demo:

```
[Inherit] C boosted to priority 3 (transitive inheritance)   <- B blocks on m2
[Inherit] B boosted to priority 5 (transitive inheritance)   <- A blocks on m1
[Inherit] C boosted to priority 5 (transitive inheritance)   <- propagates A->B->C
```

Priority restoration is recomputed on every release: a task that still holds a
mutex with waiters **keeps** the inheritance it owes to that mutex and is only
restored to its base priority after its *last* held mutex is released.

## Recursive Mutex

The same task may `acquire()` the same mutex multiple times. A hold-count is
tracked, and the mutex is only freed after a matching number of `release()`
calls:

```cpp
mutex.acquire(self);   // hold-count 1
mutex.acquire(self);   // hold-count 2  (re-entrant call)
mutex.release(self);   // hold-count 1  -> still owned by self
mutex.release(self);   // hold-count 0  -> freed / handed to a waiter
```

---

# Sample Output

> The `Tick` numbers are real-time and may vary by a tick or two between runs;
> the **sequence** of states and `[Inherit]` lines is what matters.

## 1. Lifecycle Demo (`TinyRTOS.exe`)

Four tasks walk the complete lifecycle:

| Task   | Priority | Lifecycle shown                                      |
|--------|----------|------------------------------------------------------|
| Beta   | 5        | RUNNING -> **SUSPENDED** -> READY -> TERMINATED      |
| Delta  | 4        | RUNNING -> **BLOCKED** (delay) -> READY -> TERMINATED|
| Gamma  | 3        | RUNNING -> **WAITING** (mutex) -> READY -> TERMINATED|
| Alpha  | 1        | RUNNING -> **BLOCKED** (delay) -> READY -> TERMINATED|

```
==========================================
 TinyRTOS - Complete Task Lifecycle Demo
==========================================
Task Created : Alpha | Priority = 1
Task Created : Beta | Priority = 5
Task Created : Gamma | Priority = 3
Task Created : Delta | Priority = 4

=====================================
 TinyRTOS Priority Round Robin
=====================================

---------------------------------
Tick     : 0
Running  : Beta
Priority : 5
Run No.  : 1
---------------------------------
[State] Beta -> RUNNING (run no. 1)
Beta suspends itself
[State] Beta -> SUSPENDED (paused, must be resumed)
Beta suspended - paused until resumed
---------------------------------
Tick     : 4
Running  : Delta
Priority : 4
Run No.  : 1
---------------------------------
[State] Delta -> RUNNING (run no. 1)
Delta acquires the bus
[DEBUG] Delta blocked until tick 14
Delta moved to DELAY queue
---------------------------------
Tick     : 9
Running  : Gamma
Priority : 3
Run No.  : 1
---------------------------------
[State] Gamma -> RUNNING (run no. 1)
Gamma blocks: WAITING for the bus
[Tick 14] Delta moved back to READY queue
---------------------------------
Tick     : 14
Running  : Delta
Priority : 4
Run No.  : 2
---------------------------------
[State] Delta -> RUNNING (run no. 2)
Delta releases the bus
[State] Delta -> TERMINATED (never scheduled again)
Delta removed from scheduling
---------------------------------
Tick     : 18
Running  : Gamma
Priority : 3
Run No.  : 2
---------------------------------
[State] Gamma -> RUNNING (run no. 2)
Gamma owns the bus after WAITING
Gamma resumes the suspended Beta
[State] Beta -> READY (resumed)
Beta moved back to READY queue (resumed)
[State] Gamma -> TERMINATED (never scheduled again)
Gamma removed from scheduling
---------------------------------
Tick     : 23
Running  : Beta
Priority : 5
Run No.  : 2
---------------------------------
[State] Beta -> RUNNING (run no. 2)
Beta running again after being resumed
[State] Beta -> TERMINATED (never scheduled again)
Beta removed from scheduling
---------------------------------
Tick     : 27
Running  : Alpha
Priority : 1
Run No.  : 1
---------------------------------
[State] Alpha -> RUNNING (run no. 1)
Alpha delays for 3 ticks
[DEBUG] Alpha blocked until tick 30
Alpha moved to DELAY queue
[Tick 32] Alpha moved back to READY queue
---------------------------------
Tick     : 32
Running  : Alpha
Priority : 1
Run No.  : 2
---------------------------------
[State] Alpha -> RUNNING (run no. 2)
Alpha woke up from delay
[State] Alpha -> TERMINATED (never scheduled again)
Alpha removed from scheduling

=====================================
 All tasks TERMINATED - kernel done
=====================================
```

## 2. Inheritance Demo (`TinyRTOS_Inheritance.exe`)

Chain: **A(5)** waits on `m1` held by **B(3)**; **B** waits on `m2` held by
**C(1)**. Watch the `[Inherit]` lines — C is boosted twice, and the second
boost is the **transitive** one (propagating from A through B to C):

```
==========================================
 Transitive Priority Inheritance Demo
 A(5) waits on m1 held by B(3)
 B(3) waits on m2 held by C(1)
==========================================

Task Created : A | Priority = 5
Task Created : B | Priority = 3
Task Created : C | Priority = 1

=====================================
 TinyRTOS Priority Round Robin
=====================================

---------------------------------
Tick     : 0
Running  : A
Priority : 5
Run No.  : 1
---------------------------------
[State] A -> RUNNING (run no. 1)
A delays first so B and C can build the chain
[DEBUG] A blocked until tick 20
A moved to DELAY queue
---------------------------------
Tick     : 4
Running  : B
Priority : 3
Run No.  : 1
---------------------------------
[State] B -> RUNNING (run no. 1)
B acquired m1 (holds it)
[DEBUG] B blocked until tick 14
B moved to DELAY queue
---------------------------------
Tick     : 9
Running  : C
Priority : 1
Run No.  : 1
---------------------------------
[State] C -> RUNNING (run no. 1)
C acquired m2 (holds it)
[DEBUG] C blocked until tick 34
C moved to DELAY queue
[Tick 14] B moved back to READY queue
---------------------------------
Tick     : 14
Running  : B
Priority : 3
Run No.  : 2
---------------------------------
[State] B -> RUNNING (run no. 2)
[Inherit] C boosted to priority 3 (transitive inheritance)
B blocks: WAITING on m2
No READY tasks.
[Tick 20] A moved back to READY queue
---------------------------------
Tick     : 20
Running  : A
Priority : 5
Run No.  : 2
---------------------------------
[State] A -> RUNNING (run no. 2)
[Inherit] B boosted to priority 5 (transitive inheritance)
[Inherit] C boosted to priority 5 (transitive inheritance)
A blocks: WAITING on m1
No READY tasks.
[Tick 34] C moved back to READY queue
---------------------------------
Tick     : 34
Running  : C
Priority : 5
Run No.  : 2
---------------------------------
[State] C -> RUNNING (run no. 2)
C releases m2
[Inherit] C priority restored to 1
[State] C -> TERMINATED (never scheduled again)
C removed from scheduling
---------------------------------
Tick     : 39
Running  : B
Priority : 5
Run No.  : 3
---------------------------------
[State] B -> RUNNING (run no. 3)
[Inherit] B priority restored to 3
B released m1 -> A gets it
[State] B -> TERMINATED (never scheduled again)
B removed from scheduling
---------------------------------
Tick     : 43
Running  : A
Priority : 5
Run No.  : 3
---------------------------------
[State] A -> RUNNING (run no. 3)
A finished, releasing m1
[State] A -> TERMINATED (never scheduled again)
A removed from scheduling

=====================================
 All tasks TERMINATED - kernel done
=====================================
```

Notice that **C runs at `Priority : 5`** — boosted by transitive inheritance —
and **B runs at `Priority : 5`** while it still holds `m1` (with A waiting on
it), only dropping back to 3 after its last release. This is exactly the
behavior that prevents priority inversion.

---

# Tuning the Demos

* `Scheduler scheduler(boostInterval)` — how often aging boosts low-priority
  tasks. A small value (e.g. `3`) makes low-priority tasks get served quickly;
  a large value (e.g. `100000`) effectively disables aging.
* `Task::delay(ticks)` — how long a task sleeps before returning to READY.
* Change task priorities and delays in `kernel/main.cpp` and
  `kernel/demo_inheritance.cpp` to see different scheduling patterns.
* Remove the `terminate()` calls (or add a never-ending task) to watch the
  scheduler run forever; stop it with `Ctrl + C`.

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
* Deadlock detection
* Embedded Hardware Port (STM32 / ESP32)

---

# Learning Outcomes

This project helps build an understanding of:

* Operating System Scheduling
* RTOS Design
* Task Management
* The Complete Task Lifecycle (READY / RUNNING / BLOCKED / WAITING / SUSPENDED / TERMINATED)
* Priority Scheduling
* Round Robin Scheduling
* Priority Aging (preventing starvation)
* Priority Inversion & Priority Inheritance (direct + transitive)
* Recursive Mutexes and Critical Sections
* Delay Management
* System Tick Generation
* Concurrent Programming in Modern C++
* RTOS Kernel Architecture

---

# Author

**Titus Shachin**

Built as a learning project to understand RTOS internals and operating system scheduling concepts using Modern C++.
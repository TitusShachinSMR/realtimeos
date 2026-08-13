cmake_minimum_required(VERSION 3.16)

project(TinyRTOS)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(TinyRTOS

kernel/main.cpp

kernel/Task.cpp

kernel/Scheduler.cpp

kernel/Mutex.cpp
)
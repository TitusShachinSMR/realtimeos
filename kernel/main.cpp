#include <iostream>
#include "Task.h"

#include <iostream>

#include "Task.h"
#include "Scheduler.h"

void ledTask()
{
    std::cout << "Blinking LED\n";
}

void uartTask()
{
    std::cout << "Sending UART Data\n";
}

void sensorTask()
{
    std::cout << "Reading Sensor\n";
}

void motorTask()
{
    std::cout << "Controlling Motor\n";
}
int main()
{
  Task led(
    1,
    "LED",
    2,
    [](Task& self)
    {
        std::cout << "Blinking LED\n";
        self.delay(5);
    });

  Task uart(
    2,
    "UART",
    1,
    [](Task& self)
    {
        std::cout << "Sending UART Data\n";
    });

  Task sensor(
    3,
    "Sensor",
    5,
    [](Task& self)
    {
        std::cout << "Reading Sensor\n";
    });

  Task motor(
    4,
    "Motor",
    3,
    [](Task& self)
    {
        std::cout << "Controlling Motor\n";
    });

    Scheduler scheduler;

   scheduler.createTask(&led);

  scheduler.createTask(&uart);

  scheduler.createTask(&sensor);

  scheduler.createTask(&motor);

  scheduler.start();

    return 0;
}
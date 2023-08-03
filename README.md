# rtos_scratch_cortex_m4
RTOS project using RoundRobin scheduler on stm32 arm cortex M4
## RoundRobinScheduler: 
  Profilers, essentially counters, are used to view the frequency of each task/thread that is run.
  Sample functions within main.c are disabled here. They only access the UART to print out messages. When enabled, printed messages will not print out perfectly.
  This is due to the sharing of a single resource (UART in this case), and the limited allocated time quanta given to each task.
## SpinlockSemaphore: 
  This project builds on top of the *RoundRobinScheduler*. 
  Semaphores are used to fix the sharing of resources (i.e. UART), allowing the current running thread to complete execution before switching
  to the next task.
  A Periodic task is added to this project (task3). It uses a profiler to view its execution frequency.
 

#include "uart.h"
#include "osKernel.h"

#define 	QUANTA		10
typedef uint32_t TaskProfiler;

TaskProfiler Task0_Profiler, Task1_Profiler, Task2_Profiler, pTask1_Profiler, pTask2_Profiler;

int32_t semaphore1, semaphore2;

void motor_run(void);
void motor_stop(void);
void valve_open(void);
void valve_close(void);


void task3(void)
{
	pTask1_Profiler++;
}
void task0(void)
{
	while(1)
	{
		Task0_Profiler++;
	}
}
void task1(void)
{
	while(1)
	{
		//Task1_Profiler++;
		osSemaphoreWait(&semaphore1);
		motor_run();
		osSemaphoreSet(&semaphore2);
	}
}
void task2(void)
{
	while(1)
	{
		//Task2_Profiler++;
		osSemaphoreWait(&semaphore2);  /*Initialized to 0, so this wont execute. When get here will be stuck*/
		valve_open();
		osSemaphoreSet(&semaphore1);
	}
}

int main(void)
{
	/* Initialize uart to view function outputs */
	uart_tx_init();

	/* Initialize hardware timer */
	tim2_1hz_interrupt_init();

	/* Initialize semaphores */
	osSemaphoreInit(&semaphore1,1);
	osSemaphoreInit(&semaphore2,0);

	/* Initialize kernel */
	osKernelInit();

	/* Add Threads */
	osKernelAddThreads(&task0,&task1,&task2);

	/* Set RoundRobin time quanta */
	osKernelLaunch(QUANTA);

}

void TIM2_IRQHandler(void)
{
	/* Clear update interrupt flag */
	TIM2->SR &=~SR_UIF;

	/* Do something */
	pTask2_Profiler++;
}

void motor_run(void)
{
	printf("Motor is starting...\n\r");
}

void motor_stop(void)
{
	printf("Motor is stopping...\n\r");
}

void valve_open(void)
{
	printf("Valve is opening...\n\r");
}

void valve_close(void)
{
	printf("Valve is closing...\n\r");
}

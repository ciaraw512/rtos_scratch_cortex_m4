#include "osKernel.h"

#define NUM_OF_THREADS			3
#define STACKSIZE				100  /* 100 32-bit values. i.e. 100 4-byte values (400 bytes) */

#define BUS_FREQ				16000000

#define CTRL_ENABLE		(1U<<0)
#define CTRL_TICKINT	(1U<<1)
#define CTRL_CLCKSRC	(1U<<2)
#define CTRL_COUNTFLAG	(1U<<16)
#define SYSTICK_RST		0


void osSchedulerLaunch(void);


uint32_t MILLIS_PRESCALER;

/* Thread Control Block TCB */
struct tcb{
	int32_t *stackPt;
	struct tcb *nextPt;
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_OF_THREADS];  /* Array of threads */
tcbType *currentPt;



/* Each thread with stacksize of 100 i.e. 400bytes */
int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];  /*holds stack for all threads. NUM_OF_THREADS indicates which thread stack*/


void osKernelStackInit(int i)
{
	tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE - 16];  /*Stack Pointer*/

	/* Set bit21 (T-bit) in PSR to 1 for Thumb mode (T-bit is part of EPSR within xPSR) */
	TCB_STACK[i][STACKSIZE -1] =  (1U<<24);  /*PSR*/

	/*Note: PC is not initialized here. Initialization dependent on particular thread*/

	/*Dummy stack content */
	TCB_STACK[i][STACKSIZE-3] = 0xAAAAAAAA;		/*R14 i.e. LR*/
	TCB_STACK[i][STACKSIZE-4] = 0xAAAAAAAA;		/*R12*/
	TCB_STACK[i][STACKSIZE-5] = 0xAAAAAAAA;		/*R3*/
	TCB_STACK[i][STACKSIZE-6] = 0xAAAAAAAA;		/*R2*/
	TCB_STACK[i][STACKSIZE-7] = 0xAAAAAAAA;		/*R1*/
	TCB_STACK[i][STACKSIZE-8] = 0xAAAAAAAA;		/*R0*/

	TCB_STACK[i][STACKSIZE-9] = 0xAAAAAAAA;		/*R11*/
	TCB_STACK[i][STACKSIZE-10] = 0xAAAAAAAA;	/*R10*/
	TCB_STACK[i][STACKSIZE-11] = 0xAAAAAAAA;	/*R9*/
	TCB_STACK[i][STACKSIZE-12] = 0xAAAAAAAA;	/*R8*/
	TCB_STACK[i][STACKSIZE-13] = 0xAAAAAAAA;	/*R7*/
	TCB_STACK[i][STACKSIZE-14] = 0xAAAAAAAA;	/*R6*/
	TCB_STACK[i][STACKSIZE-15] = 0xAAAAAAAA;	/*R5*/
	TCB_STACK[i][STACKSIZE-16] = 0xAAAAAAAA;	/*R4*/

}

uint8_t osKernelAddThreads(void(*task0)(void), void(*task1)(void), void(*task2)(void))
{
	/* Disable global interrupts */
	__disable_irq();

	/* link list pointing next pointer to address of next thread */
	tcbs[0].nextPt = &tcbs[1];
	tcbs[1].nextPt = &tcbs[2];
	tcbs[2].nextPt = &tcbs[0];

	/* Initialize stack for thread0 */
	osKernelStackInit(0);
	/* Initialize PC */
	TCB_STACK[0][STACKSIZE-2] = (int32_t)(task0);  /*takes address of task0 function*/

	/* Initialize stack for thread1 */
	osKernelStackInit(1);
	/* Initialize PC */
	TCB_STACK[1][STACKSIZE-2] = (int32_t)(task1);  /*takes address of task1 function*/

	/* Initialize stack for thread2 */
	osKernelStackInit(2);
	/* Initialize PC */
	TCB_STACK[2][STACKSIZE-2] = (int32_t)(task2);  /*takes address of task2 function*/

	/* Tell RTOS to start from thread 0 */
	currentPt = &tcbs[0];

	/* Enable global interrupt */
	__enable_irq();

	return 1;
}

void osKernelInit(void)
{
	MILLIS_PRESCALER = (BUS_FREQ/1000);  /* convert seconds to millisec divide 16MHz clock by 1000 */
}

void osKernelLaunch(uint32_t quanta)
{
	/* Reset SysTick */
	SysTick->CTRL = SYSTICK_RST;

	/* Clear SysTick current value register */
	SysTick->VAL = 0;

	/* Load quanta */
	SysTick->LOAD = (quanta * MILLIS_PRESCALER) - 1;

	/* Set SysTick to low priority */
	NVIC_SetPriority(SysTick_IRQn,15);

	/* Enable SysTick and Select internal clock */
	SysTick->CTRL = CTRL_CLCKSRC | CTRL_ENABLE;

	/* Enable SysTick interrupt */
	SysTick->CTRL |= CTRL_TICKINT;

	/* Launch scheduler */
	osSchedulerLaunch();
}

/* When exception occurs these registers are automatically
 * saved onto the stack : r0,r1,r2,r3,r12,lr,pc,psr */
__attribute__((naked)) void SysTick_Handler(void)
{
	/* SUSPEND CURRENT THREAD */

	/* Disable global interrupts */
	__asm("CPSID I");

	/* Save r4,r5,r6,r7,r8,r9,r10,r11 */
	__asm("PUSH {R4-R11}");

	/* Load the value at address of currentPt into R0 */
	__asm("LDR R0, =currentPt");

	/* Load into R1 from address pointed to by R0, i.e, R1 = currentPt */
	__asm("LDR R1,[R0]");

	/* Store cortex-M SP at address equals R1, i.e. Save SP into tcb */
	__asm("STR SP,[R1]");

	/* CHOOSE NEXT THREAD */

	/* Load R1 from a location 4bytes above address R1. i.e. R1 = currentPt->next */
	__asm("LDR R1,[R1,#4]");

	/* Store R1 at address equals R0, i.e. currentPt = R1 */
	__asm("STR R1,[R0]");

	/* Load Cortex-M SP from address equals R1, i.e SP = currentPt->stackPt */
	__asm("LDR SP,[R1]");

	/* Restore R4,R5,R6,R7,R8,R9,R10,R11 */
	__asm("POP {R4-R11}");

	/* Enable global interrupts */
	__asm("CPSIE I");

	/* Return from exception and restore R0,R1,R2,R3,R12,LR,PC,PSR */
	__asm("BX LR");

}

void osSchedulerLaunch(void)
{
	/* Load address of currentPt into R0 */
	__asm("LDR R0,=currentPt");

	/* Load R2 from address equals R0, i.e R2=currentPt */
	__asm("LDR R2,[R0]");

	/* Load Cortex-M SP from address equals R2, i.e SP = currentPt->stackPt */
	__asm("LDR SP,[R2]");

	/* Restore R4,r5,r6,r7,r8,r9,r10,r11 */
	__asm("POP {R4-R11}");

	/* Restore R12 */
	__asm("POP {R12}");

	/* Restore r0,r1,r2,r3 */
	__asm("POP {R0-R3}");

	/* Skip LR and PSR by adding 4 to SP */
	__asm("ADD SP,SP,#4");

	/* Create a new start location by popping LR */
	__asm("POP {LR}");

	/* Skip PSR by adding 4 to SP */
	__asm("ADD SP,SP,#4");

	/* Enable global interrupt */
	__asm("CPSIE I");

	/* Return from exception */
	__asm("BX LR");
}

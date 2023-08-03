#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "stm32f4xx.h"

#define GPIODEN			(1U<<3)
#define UART2EN			(1U<<17)

#define SYS_FREQ		16000000
#define APB1_CLK		SYS_FREQ
#define UART_BAUDRATE	115200

#define CR1_TE			(1U<<3)
#define CR1_UE			(1U<<13)

#define SR_TXE			(1U<<7)

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate);
static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);
static void uart_write(int ch);


/* this function allows the use of printf */
int __io_putchar(int ch)
{
	uart_write(ch);
	return ch;
}
void uart_tx_init(void)
{
	/* Enable clock access to GPIOD */
	RCC->AHB1ENR |= GPIODEN;

	/* Set PD5 mode to alt func mode */
	GPIOD->MODER &= ~(1U<<10);
	GPIOD->MODER |= (1U<<11);

	/* Set alt func type to AF7 (USART2_TX). AFRL5(PD5-pin 5) to AF7:0111 */
	GPIOD->AFR[0] |= (1U<<20);
	GPIOD->AFR[0] |= (1U<<21);
	GPIOD->AFR[0] |= (1U<<22);
	GPIOD->AFR[0] &= ~(1U<<23);

	/* Enable clock access to UART2: APB1 bus. RCC APB1 bit 17 USART2EN */
	RCC->APB1ENR |= UART2EN;

	/* Configure baudrate */
	uart_set_baudrate(APB1_CLK, UART_BAUDRATE);

	/* Configure transfer direction: control register - USART_CR (TE to enable transmitter) */
	USART2->CR1 = CR1_TE;  /*Clear everything. Set CR1 to TE*/

	/* Enable UART module: control register - USART enable "UE": bit 13 set to 1 */
	USART2->CR1 |= CR1_UE;
}

static void uart_write(int ch)
{
	/* Make sure transmit data register empty - check status register bit(usart_sr: TXE) */
	while (!(USART2->SR & SR_TXE)){}	/*wait till able to transmit: & SR_TXE - if result is zero then one of the operands is zero */
	/* write to transmit data register */
	USART2->DR = (ch & 0XFF);
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate)
{
	USART2->BRR = compute_uart_bd(periph_clk, baudrate);
}

static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baudrate)
{
	return ((periph_clk + (baudrate/2U))/baudrate);
}

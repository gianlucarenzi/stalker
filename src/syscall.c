/*
 * Syscall
 */

#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "syscall.h"
#include <stm32f4xx_hal.h> // per HAL_StatusTypeDef
#include "debug.h"

static t_syscall_status uart_initialize = SYSCALL_NOTREADY;
static UART_HandleTypeDef *uart = NULL;
static int debuglevel = DBG_ERROR;

/**
 * @brief  Sets the UART handle for the syscalls.
 * @param  rdy: The status of the UART.
 * @param  ptr: A pointer to the UART handle.
 * @retval None
 */
void _write_ready(t_syscall_status rdy, UART_HandleTypeDef *ptr)
{
	if (ptr != NULL)
	{
		uart_initialize = rdy;
		uart = ptr;
	}
}

/**
 * @brief  Writes a block of data to the UART.
 * @param  file: The file descriptor.
 * @param  data: A pointer to the data to write.
 * @param  len: The length of the data to write.
 * @retval The number of bytes written.
 */
int _write(int file, char *data, int len)
{
	HAL_StatusTypeDef status = HAL_OK - 1;

	if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
	{
		errno = EBADF;
		return -1;
	}

	if (uart_initialize == SYSCALL_READY)
	{
		if (uart != NULL)
		{
			// arbitrary timeout 1000
			status = HAL_UART_Transmit(uart, (uint8_t*)data, len, 1000);
		}
	}

	// return # of bytes written - as best we can tell
	return (status == HAL_OK ? len : 0);
}

static uint32_t timertick_start_ms = 0;

/**
 * @brief  Starts a timer.
 * @retval None
 */
void timer_start(void)
{
	// When timer starts get the realtime system tick
	timertick_start_ms = HAL_GetTick();
}

/**
 * @brief  Checks if a timer has elapsed.
 * @param  msec: The number of milliseconds to check.
 * @retval 1 if the timer has elapsed, 0 otherwise.
 */
int timer_elapsed(uint32_t msec)
{
	int retval;
	uint32_t ticks = HAL_GetTick();

	if (ticks < (timertick_start_ms + msec))
		retval = 0;
	else
		retval = 1;

	return retval;
}

// I hate this delay because they are clockspeed dependent!!!
#define delayUS_ASM(us) do {
	asm volatile ( 	"MOV R0,%[loops]\n\t"
			"1: \n\t"
			"SUB R0, #1\n\t"
			"CMP R0, #0\n\t"
			"BNE 1b \n\t" : : [loops] "r" (16*us) : "memory"
		      );
} while(0)

/**
 * @brief  Delays for a number of microseconds.
 * @param  micros: The number of microseconds to delay.
 * @retval None
 */
void udelay(uint32_t micros)
{
	DBG_N("Enter with: %lu\n", micros);
	if (micros > 0)
	{
		/* Go to number of cycles for system */
		DBG_N("MICROS: %lu\r\n", micros);
		delayUS_ASM(micros);
	}
	DBG_N("Exit\r\n");
}

/**
 * @brief  Delays for a number of milliseconds.
 * @param  millis: The number of milliseconds to delay.
 * @retval None
 */
void mdelay(uint32_t millis)
{
	HAL_Delay(millis);
}
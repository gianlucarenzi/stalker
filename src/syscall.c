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
#define delayUS_ASM(us) do {\
	asm volatile ( 	"MOV R0,%[loops]\n\t"\
			"1: \n\t"\
			"SUB R0, #1\n\t"\
			"CMP R0, #0\n\t"\
			"BNE 1b \n\t" : : [loops] "r" (16*us) : "memory"\
		      );\
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

/**
 * Pointer to the current high watermark of the heap usage
 */
static uint8_t *__sbrk_heap_end = NULL;

/**
 * @brief _sbrk() allocates memory to the newlib heap and is used by malloc
 *        and others from the C library
 *
 * @verbatim
 * ############################################################################
 * #  .data  #  .bss  #       newlib heap       #          MSP stack          #
 * #         #        #                         # Reserved by _Min_Stack_Size #
 * ############################################################################
 * ^-- RAM start      ^-- _end                             _estack, RAM end --^
 * @endverbatim
 *
 * This implementation starts allocating at the '_end' linker symbol
 * The '_Min_Stack_Size' linker symbol reserves a memory for the MSP stack
 * The implementation considers '_estack' linker symbol to be RAM end
 * NOTE: If the MSP stack, at any point during execution, grows larger than the
 * reserved size, please increase the '_Min_Stack_Size'.
 *
 * @param incr Memory size
 * @return Pointer to allocated memory
 */
void *_sbrk(ptrdiff_t incr)
{
	extern uint8_t _end; /* Symbol defined in the linker script */
	extern uint8_t _estack; /* Symbol defined in the linker script */
	extern uint32_t _Min_Stack_Size; /* Symbol defined in the linker script */
	const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
	const uint8_t *max_heap = (uint8_t *)stack_limit;
	uint8_t *prev_heap_end;

	/* Initialize heap end at first call */
	if (NULL == __sbrk_heap_end)
	{
		__sbrk_heap_end = &_end;
	}

	/* Protect heap from growing into the reserved MSP stack */
	if (__sbrk_heap_end + incr > max_heap)
	{
		errno = ENOMEM;
		return (void *)-1;
	}

	prev_heap_end = __sbrk_heap_end;
	__sbrk_heap_end += incr;

	return (void *)prev_heap_end;
}

#if defined(__PICOLIBC__)
	// Picolibc expects syscalls without the leading underscore.
	// This creates a strong alias so that
	// calls to `sbrk()` are resolved to our `_sbrk()` implementation.
	__strong_reference(_sbrk, sbrk);
#endif

/*
 * delay_us.c
 *
 *  Created on: Nov 11, 2025
 *      Author: Gianluca Renzi <icjtqr@gmail.com>
 */

#include "delay_us.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

// DWT (Data Watchpoint and Trace) registers
#define DWT_CONTROL             (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT              (*(volatile uint32_t*)0xE0001004)
#define DEM_CR                  (*(volatile uint32_t*)0xE000EDFC)
#define DEM_CR_TRCENA           (1 << 24)
#define DWT_CTRL_CYCCNTENA      (1 << 0)

static uint32_t s_dwt_initialized = 0;
static uint32_t s_cycles_per_us = 0;

/**
 * @brief Initializes the DWT cycle counter for microsecond delays.
 */
void DelayUs_Init(void)
{
	// Enable access to DWT registers
	DEM_CR |= DEM_CR_TRCENA;

	// Reset the cycle counter
	DWT_CYCCNT = 0;

	// Enable the cycle counter
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA;

	// Calculate cycles per microsecond based on system clock
	// SystemCoreClock is defined by CMSIS and contains the current clock frequency
	s_cycles_per_us = SystemCoreClock / 1000000;

	s_dwt_initialized = 1;
}

/**
 * @brief Delays execution for the specified number of microseconds.
 * @param us: Delay time in microseconds
 */
void DelayUs(uint32_t us)
{
	// If DWT not initialized, fall back to HAL_Delay (less precise)
	if (!s_dwt_initialized)
	{
		// Convert microseconds to milliseconds (round up)
		uint32_t ms = (us + 999) / 1000;
		if (ms > 0)
		{
			HAL_Delay(ms);
		}
		return;
	}

	// Calculate target cycles
	uint32_t target_cycles = us * s_cycles_per_us;

	// Start timing
	uint32_t start = DWT_CYCCNT;

	// Busy-wait until target cycles elapsed
	while ((DWT_CYCCNT - start) < target_cycles)
	{
		// Busy wait - this is intentional for precise timing
		__NOP();
	}
}

/**
 * @brief Linux-style microsecond delay function.
 * @param us: Delay time in microseconds
 */
void udelay(uint32_t us)
{
	DelayUs(us);
}

/**
 * @brief Gets the current value of the DWT cycle counter.
 * @return Current cycle count value
 */
uint32_t DelayUs_GetCycles(void)
{
	if (!s_dwt_initialized)
	{
		return 0;
	}
	return DWT_CYCCNT;
}

/**
 * @brief Calculates elapsed microseconds between two cycle counter readings.
 * @param start: Starting cycle count
 * @param end: Ending cycle count
 * @return Elapsed time in microseconds
 */
uint32_t DelayUs_ElapsedUs(uint32_t start, uint32_t end)
{
	if (!s_dwt_initialized || s_cycles_per_us == 0)
	{
		return 0;
	}

	// Handle counter overflow (it's a 32-bit counter that wraps around)
	uint32_t elapsed_cycles;
	if (end >= start)
	{
		elapsed_cycles = end - start;
	}
	else
	{
		// Counter wrapped around
		elapsed_cycles = (0xFFFFFFFF - start) + end + 1;
	}

	return elapsed_cycles / s_cycles_per_us;
}

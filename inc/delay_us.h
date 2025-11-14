/*
 * delay_us.h
 *
 *  Created on: Nov 11, 2025
 *      Author: Gianluca Renzi <icjtqr@gmail.com>
 */

#ifndef INC_DELAY_US_H_
#define INC_DELAY_US_H_

#include <stdint.h>

/**
 * @brief Initializes the DWT (Data Watchpoint and Trace) unit for microsecond delays.
 * @note  Must be called once during initialization, after SystemClock_Config().
 * @note  DWT is available on Cortex-M3/M4/M7 cores.
 */
extern void DelayUs_Init(void);

/**
 * @brief Delays execution for the specified number of microseconds.
 * @note  This is a busy-wait delay that blocks the CPU.
 * @note  NOT suitable for use in RTOS tasks for delays > 1ms - use osDelay() instead.
 * @note  Suitable for:
 *        - Short hardware timing delays (< 1ms)
 *        - Interrupt context (ISR-safe)
 *        - Bit-banging protocols
 *        - Precise hardware control sequences
 * @param us: Delay time in microseconds (1-1000000)
 */
extern void DelayUs(uint32_t us);

/**
 * @brief Delays execution for the specified number of microseconds (Linux-style name).
 * @note  This is an alias for DelayUs() for compatibility with Linux kernel style.
 * @param us: Delay time in microseconds (1-1000000)
 */
extern void udelay(uint32_t us);

/**
 * @brief Gets the current value of the DWT cycle counter.
 * @return Current cycle count value
 * @note  Useful for precise timing measurements and profiling
 */
extern uint32_t DelayUs_GetCycles(void);

/**
 * @brief Calculates elapsed microseconds between two cycle counter readings.
 * @param start: Starting cycle count (from DelayUs_GetCycles())
 * @param end: Ending cycle count (from DelayUs_GetCycles())
 * @return Elapsed time in microseconds
 * @note  Handles counter overflow correctly
 */
extern uint32_t DelayUs_ElapsedUs(uint32_t start, uint32_t end);

#endif /* INC_DELAY_US_H_ */

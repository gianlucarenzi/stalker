/*
 * log_task.h
 *
 *  Created on: Nov 11, 2025
 *      Author: Gemini
 */

#ifndef INC_LOG_TASK_H_
#define INC_LOG_TASK_H_

#include "stm32f4xx_hal.h"

/**
 * @brief Initializes the logging task and its queue.
 *
 * This function must be called after the UART peripheral is initialized but before
 * the FreeRTOS scheduler is started.
 *
 * @param huart Pointer to the UART_HandleTypeDef structure that will be used for logging.
 */
void LogTask_Init(UART_HandleTypeDef *huart);

/**
 * @brief Sends a block of data to the logging system.
 *
 * This function checks the state of the FreeRTOS scheduler.
 * - If the scheduler is running, it queues the data to be sent by the LogTask.
 * - If the scheduler is not running, it sends the data directly via UART (blocking).
 *
 * This function is intended to be called by the `_write` syscall.
 *
 * @param ptr Pointer to the data buffer.
 * @param len Length of the data in bytes.
 * @return The number of bytes written.
 */
int LogTask_SendString(const char *ptr, int len);

/**
 * @brief Printf-like function that sends data directly to log queue without prefix
 *
 * This function bypasses the normal printf mechanism and sends data directly
 * to the log queue without adding the "> " prefix. This prevents deadlock when
 * called from within DBG_* macros which already use printf internally.
 *
 * @param format Format string (printf-style)
 * @param ... Variable arguments
 * @return Number of characters written, or negative on error
 */
int log_printf(const char *format, ...);

#endif /* INC_LOG_TASK_H_ */

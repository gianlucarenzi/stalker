/*
 * log_task.c
 *
 *  Created on: Nov 11, 2025
 *      Author: Gianluca Renzi
 */

#include "log_task.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

// Module-level static variables
static osMessageQueueId_t s_logQueue = NULL;
static UART_HandleTypeDef *s_logHuart = NULL;

osThreadId_t logTaskHandle;
const osThreadAttr_t logTask_attributes = {
  .name = "LogTask",
  .stack_size = 256,
  .priority = (osPriority_t) osPriorityLow,
};

// Queue settings
#define LOG_QUEUE_LENGTH        512
#define LOG_QUEUE_ITEM_SIZE     sizeof(char)

/**
 * @brief The core logging task. Transmits one character at a time.
 *
 * This simplified version waits forever for a character and transmits it
 * immediately using a blocking call. This has been found to be the most
 * stable implementation for this environment.
 *
 * @param argument Not used.
 */
static void LogTask(void *argument) {
    (void)argument;
    char received_char;
    osStatus_t status;

    for (;;) {
        // Wait forever for a character from the queue
        status = osMessageQueueGet(s_logQueue, &received_char, NULL, osWaitForever);
        if (status == osOK) {
            // Transmit one character at a time.
            if (s_logHuart != NULL) {
                HAL_UART_Transmit(s_logHuart, (uint8_t *)&received_char, 1, HAL_MAX_DELAY);
            }
        }
    }
}

/**
 * @brief Initializes the logging task and its queue.
 */
void LogTask_Init(UART_HandleTypeDef *huart)
{
    s_logHuart = huart;

    // Create the message queue to hold log messages
    s_logQueue = osMessageQueueNew(LOG_QUEUE_LENGTH, LOG_QUEUE_ITEM_SIZE, NULL);
    configASSERT(s_logQueue);

    // Create the logging task
    logTaskHandle = osThreadNew(LogTask, NULL, &logTask_attributes);
    configASSERT(logTaskHandle);
}

/**
 * @brief Sends a block of data to the logging system.
 */
int LogTask_SendString(const char *ptr, int len) {
    if (ptr == NULL || len < 0) {
        return -1;
    }

    // Use a timeout of 0 if we are in an ISR, otherwise use a small timeout.
    // This makes the call non-blocking from an ISR, which is required.
    uint32_t timeout = ( __get_IPSR() != 0U ) ? 0U : 10U;

    // If scheduler is not running, write directly to UART using blocking method (baremetal).
    if (osKernelGetState() != osKernelRunning) {
        if (s_logHuart != NULL) {
            HAL_UART_Transmit(s_logHuart, (uint8_t *)ptr, len, HAL_MAX_DELAY);
        }
        return len;
    }

    // If scheduler is running, send to queue with "> " prefix for RTOS messages
    if (s_logQueue == NULL) {
        return -1;
    }

    // Track if we need to add prefix at the beginning of a new line
    static bool needPrefix = true;

    for (int i = 0; i < len; i++) {
        // Add "> " prefix at the start of each new line (RTOS context)
        if (needPrefix) {
            char prefix[2] = {'>', ' '};
            for (int p = 0; p < 2; p++) {
                if (osMessageQueuePut(s_logQueue, &prefix[p], 0, timeout) != osOK) {
                    return i;
                }
            }
            needPrefix = false;
        }

        // Post character to the queue.
        if (osMessageQueuePut(s_logQueue, &ptr[i], 0, timeout) != osOK) {
            // Failed to queue the character (e.g. queue full from ISR),
            // return the number of bytes written so far.
            return i;
        }

        // Check if this is a newline - next character will need prefix
        if (ptr[i] == '\n') {
            needPrefix = true;
        }
    }

    return len;
}

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
int log_printf(const char *format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (len < 0) {
		return -1;
	}

	if (len >= (int)sizeof(buffer)) {
		len = sizeof(buffer) - 1;
	}

	// If scheduler is not running, write directly to UART
	if (osKernelGetState() != osKernelRunning) {
		if (s_logHuart != NULL) {
			HAL_UART_Transmit(s_logHuart, (uint8_t *)buffer, len, HAL_MAX_DELAY);
		}
		return len;
	}

	// If scheduler is running, send to queue WITHOUT prefix
	if (s_logQueue == NULL) {
		return -1;
	}

	uint32_t timeout = (__get_IPSR() != 0U) ? 0U : 10U;

	// Send characters directly to queue without prefix logic
	for (int i = 0; i < len; i++) {
		if (osMessageQueuePut(s_logQueue, &buffer[i], 0, timeout) != osOK) {
			return i;
		}
	}

	return len;
}

// NOTE: The HAL_UART_TxCpltCallback is not needed for this blocking implementation.
// If it were defined, it would need to be moved or guarded to avoid conflicts.

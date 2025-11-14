/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : led_manager_task.h
  * @brief          : Header for led_manager_task.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LED_MANAGER_TASK_H
#define __LED_MANAGER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief LED command types
 */
typedef enum {
	LED_CMD_OFF = 0,        /**< Turn LED off */
	LED_CMD_ON,             /**< Turn LED on */
	LED_CMD_TOGGLE,         /**< Toggle LED state */
	LED_CMD_BLINK_BYTE,     /**< Blink LED showing byte bits (8 blinks) */
	LED_CMD_BLINK_FAST,     /**< Fast blink (for errors) */
	LED_CMD_BLINK_SLOW,     /**< Slow blink (for status) */
} led_command_t;

/**
 * @brief LED message structure
 */
typedef struct {
	led_command_t command;  /**< LED command */
	uint8_t data;           /**< Data byte (for BLINK_BYTE command) */
	uint16_t duration_ms;   /**< Duration in milliseconds (for timed commands) */
} led_manager_message_t;

/* Exported constants --------------------------------------------------------*/
#define BLINK_MS_RATE    5L
/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  LED Manager Task entry point
  * @param  argument: Not used
  * @retval None
  */
extern void LedManagerTask(void *argument);

/* External queue handle */
extern osMessageQueueId_t ledManagerQueueHandle;

#ifdef __cplusplus
}
#endif

#endif /* __LED_MANAGER_TASK_H */

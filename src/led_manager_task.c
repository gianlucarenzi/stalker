/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : led_manager_task.c
  * @brief          : LED Manager Task Implementation
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

/* Includes ------------------------------------------------------------------*/
#include "led_manager_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "debug.h"

/* External variables --------------------------------------------------------*/
extern osMessageQueueId_t ledManagerQueueHandle;

/* Private variables ---------------------------------------------------------*/
static int debuglevel = DBG_INFO;

/* Private function prototypes -----------------------------------------------*/
static void led_blink_byte(uint8_t byte);
static void led_set(GPIO_PinState state);

/**
  * @brief  Function implementing the LED Manager Task thread.
  * @param  argument: Not used
  * @retval None
  */
void LedManagerTask(void *argument)
{
	led_manager_message_t msg;
	osStatus_t status;

	DBG_V("LED Manager Task started\r\n");

	// LED off initially
	led_set(GPIO_PIN_SET);  // LED is active low on TP1

	/* Infinite loop */
	for(;;)
	{
		// Wait for LED command (blocking)
		status = osMessageQueueGet(ledManagerQueueHandle, &msg, NULL, osWaitForever);

		if (status == osOK)
		{
			switch(msg.command)
			{
				case LED_CMD_OFF:
					led_set(GPIO_PIN_SET);  // LED off (active low)
					break;

				case LED_CMD_ON:
					led_set(GPIO_PIN_RESET);  // LED on (active low)
					break;

				case LED_CMD_TOGGLE:
					HAL_GPIO_TogglePin(GPIOA, TP1_Pin);
					break;

				case LED_CMD_BLINK_BYTE:
					// Blink LED showing each bit of the byte
					led_blink_byte(msg.data);
					break;

				case LED_CMD_BLINK_FAST:
					// Fast blink pattern
					for (int i = 0; i < 6; i++)
					{
						HAL_GPIO_TogglePin(GPIOA, TP1_Pin);
						osDelay(50);
					}
					led_set(GPIO_PIN_SET);  // LED off at end
					break;

				case LED_CMD_BLINK_SLOW:
					// Slow blink pattern
					for (int i = 0; i < 4; i++)
					{
						HAL_GPIO_TogglePin(GPIOA, TP1_Pin);
						osDelay(250);
					}
					led_set(GPIO_PIN_SET);  // LED off at end
					break;

				default:
					DBG_E("Unknown LED command: %d\r\n", msg.command);
					break;
			}
		}
	}
}

/**
  * @brief  Set LED state
  * @param  state: GPIO_PIN_SET (off) or GPIO_PIN_RESET (on)
  * @retval None
  */
static void led_set(GPIO_PinState state)
{
	HAL_GPIO_WritePin(GPIOA, TP1_Pin, state);
}

/**
  * @brief  Quick blink pattern (010) to indicate data transmission
  * @param  byte: Byte to visualize (unused, just for API compatibility)
  * @retval None
  */
static void led_blink_byte(uint8_t byte)
{
	(void)byte;  // Unused parameter

	// Quick 010 pattern: OFF-ON-OFF
	led_set(GPIO_PIN_SET);    // OFF
	osDelay(BLINK_MS_RATE);
	led_set(GPIO_PIN_RESET);  // ON
	osDelay(BLINK_MS_RATE);
	led_set(GPIO_PIN_SET);    // OFF
	osDelay(BLINK_MS_RATE);

	// LED back on (since keyboard is connected)
	led_set(GPIO_PIN_RESET);
}

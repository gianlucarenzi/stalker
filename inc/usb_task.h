/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_task.h
  * @brief          : Header for usb_task.c file.
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
#ifndef __USB_TASK_H
#define __USB_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

#define KEY_PRESSED_MAX 6
typedef struct {
	int lctrl;
	int lctrlpressed;
	int lshift;
	int lshiftpressed;
	int lalt;
	int laltpressed;
	int lgui;
	int lguipressed;
	int rctrl;
	int rctrlpressed;
	int rshift;
	int rshiftpressed;
	int ralt;
	int raltpressed;
	int rgui;
	int rguipressed;
	uint8_t keys[KEY_PRESSED_MAX];
	uint8_t keyspressed[KEY_PRESSED_MAX];
} keyboard_code_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  USB Keyboard Task entry point
  * @param  argument: Not used
  * @retval None
  */
void usbKeyboardTask(void *argument);

/* External queue handles that this task uses --------------------------------*/
extern osMessageQueueId_t keyboardQueueHandle;  /**< USB -> Amiga keyboard data */
extern osMessageQueueId_t ledQueueHandle;       /**< Amiga -> USB LED status */

/* Communication structures */
typedef struct {
	uint8_t keys[6];         /**< Raw HID keys array (up to 6 simultaneous keys) */
	uint8_t modifiers;       /**< Modifier keys bitmap */
	uint32_t timestamp;      /**< Timestamp in milliseconds */
	uint8_t is_reset_event;  /**< 1 = reset event (disconnect), 0 = normal key state */
} keyboard_message_t;

typedef enum {
	NO_LED = 0,
	LED_CAPS_LOCK_ON,
	LED_NUM_LOCK_ON,
	LED_SCROLL_LOCK_ON,
	LED_CAPS_LOCK_OFF,
	LED_NUM_LOCK_OFF,
	LED_SCROLL_LOCK_OFF,
	LED_RESET_BLINK,
} led_status_t;

typedef enum {
	NUM_LOCK_LED = (1 << 0),
	CAPS_LOCK_LED = (1 << 1),
	SCROLL_LOCK_LED = (1 << 2),
} keyboard_led_t;

typedef struct {
	led_status_t led_status;
	uint32_t timestamp;
} led_message_t;


#ifdef __cplusplus
}
#endif

#endif /* __USB_TASK_H */

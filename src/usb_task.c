/**
  ******************************************************************************
  * @file           : usb_task.c
  * @brief          : USB Task Implementation for FreeRTOS-based USB to Amiga Adapter
  ******************************************************************************
  * @details        This file implements the USB task responsible for:
  *                 - USB HID keyboard communication and management
  *                 - USB device connection/disconnection handling
  *                 - Keyboard LED control and status management
  *                 - Inter-task communication with Amiga task via FreeRTOS queues
  *                 - Status LED indication for different USB states
  *
  * @author         Gianluca Renzi R.G. (RetroBitLab Tech Guy)
  * @version        v1.5-rtos
  * @date           2024
  * @copyright      (C) Copyright 2019/2024 by Gianluca Renzi
  * @license        SPDX-License-Identifier: LGPL-3.0-or-later
  *
  * @note           This task runs at high priority (USB_TASK_PRIORITY = 3) to ensure
  *                 responsive USB communication and minimal latency in keyboard handling.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_task.h"
#include "debug.h"
#include "syscall.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @brief Number of retry attempts for USB HID reports */
#define USB_REPORT_RETRY    6

/** @brief Delay before LED initialization after keyboard connection (ms) */
#define LED_INIT_DELAY_MS   500

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @brief Debug level for USB task logging */
static int debuglevel = DBG_INFO;

/** @brief FreeRTOS task handle for USB task */
TaskHandle_t usb_task_handle = NULL;

/** @brief Current state of the USB task */
static task_state_t usb_task_state = TASK_STATE_INIT;

/** @brief USB Host handle for HID communication */
static USBH_HandleTypeDef *usbhost = NULL;

/** @brief Current USB application state */
static ApplicationTypeDef usb_app_state = APPLICATION_DISCONNECT;

/** @brief Flag indicating if USB host is initialized */
static int usb_initialized = 0;

/** @brief Flag indicating if keyboard is ready for operation */
static int keyboard_ready = 0;

/** @brief Current keyboard LED state bitmask */
static keyboard_led_t current_keyboard_led = 0;

/** @brief Timestamp of last LED initialization */
static TickType_t last_led_init_time = 0;

/* Private function prototypes -----------------------------------------------*/
static void usb_task_process_keyboard(void);
static void usb_task_handle_led_messages(void);
static void usb_task_handle_connection_state(void);
static void led_toggle_status(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  USB Task main function
  * @details Main task loop that handles USB HID communication, device state management,
  *          and inter-task communication. Runs at 10ms intervals for responsive operation.
  *          Task responsibilities:
  *          - USB Host processing and state management
  *          - Keyboard data acquisition and forwarding to Amiga task
  *          - LED message processing from Amiga task
  *          - Status indication via GPIO LED
  * @param  pvParameters: Task parameters (unused)
  * @retval None (task never returns)
  * @note   Task priority: USB_TASK_PRIORITY (3 - High)
  *         Stack size: USB_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)
  */
void usb_task(void *pvParameters)
{
	TickType_t last_wake_time;
	const TickType_t task_frequency = pdMS_TO_TICKS(10); // 10ms cycle
	
	DBG_I("USB Task started\r\n");
	
	usb_task_state = TASK_STATE_RUNNING;
	
	/* Initialize the task */
	usb_task_init();
	
	/* Initialize the xLastWakeTime variable with the current time */
	last_wake_time = xTaskGetTickCount();
	
	for (;;)
	{
		/* Handle USB Host processing */
		if (usb_initialized)
		{
			MX_USB_HOST_Process();
			usb_app_state = USBH_ApplicationState();
		}
		
		/* Handle connection state changes */
		usb_task_handle_connection_state();
		
		/* Process keyboard data if available */
		if (usb_app_state == APPLICATION_READY && keyboard_ready)
		{
			usb_task_process_keyboard();
		}
		
		/* Handle LED messages from Amiga task */
		usb_task_handle_led_messages();
		
		/* Wait for the next cycle */
		vTaskDelayUntil(&last_wake_time, task_frequency);
	}
}

/**
  * @brief  Initialize USB Task
  * @details Performs USB task initialization including USB Host stack initialization.
  *          Sets up the USB OTG FS peripheral for HID keyboard communication.
  * @param  None
  * @retval None
  * @note   Called once during task startup before entering main loop
  */
void usb_task_init(void)
{
	DBG_N("USB Task initialization\r\n");
	
	/* Initialize USB HOST OTG FS */
	MX_USB_HOST_Init();
	usb_initialized = 1;
	
	DBG_N("USB Task initialization complete\r\n");
}

/**
  * @brief  Handle USB connection state changes and status indication
  * @details Manages USB device connection/disconnection events and provides visual
  *          feedback through status LED blinking patterns:
  *          - APPLICATION_READY: Solid LED, keyboard initialization
  *          - APPLICATION_DISCONNECT: Slow blink (500ms)
  *          - Other states: Fast blink (100ms)
  * @param  None
  * @retval None
  * @note   Automatically initializes keyboard LEDs after connection delay
  */
static void usb_task_handle_connection_state(void)
{
	static ApplicationTypeDef prev_state = APPLICATION_IDLE;
	static TickType_t state_change_time = 0;
	static int blink_count = 0;
	
	if (prev_state != usb_app_state)
	{
		state_change_time = xTaskGetTickCount();
		blink_count = 0;
		prev_state = usb_app_state;
		
		switch (usb_app_state)
		{
			case APPLICATION_READY:
				DBG_I("USB Keyboard connected and ready\r\n");
				usbhost = USBH_GetHost();
				if (usbhost != NULL && USBH_HID_GetDeviceType(usbhost) == HID_KEYBOARD)
				{
					keyboard_ready = 0; // Will be set to 1 after LED init
					last_led_init_time = xTaskGetTickCount();
				}
				break;
				
			case APPLICATION_DISCONNECT:
				DBG_I("USB Keyboard disconnected\r\n");
				keyboard_ready = 0;
				usbhost = NULL;
				break;
				
			default:
				DBG_N("USB Application state: %d\r\n", usb_app_state);
				keyboard_ready = 0;
				break;
		}
	}
	
	/* Handle LED blinking based on state */
	TickType_t current_time = xTaskGetTickCount();
	
	switch (usb_app_state)
	{
		case APPLICATION_READY:
			if (!keyboard_ready && usbhost != NULL)
			{
				/* Wait for LED initialization delay */
				if ((current_time - last_led_init_time) >= pdMS_TO_TICKS(LED_INIT_DELAY_MS))
				{
					DBG_V("Initializing keyboard LEDs\r\n");
					usb_keyboard_led_init_sequence(usbhost);
					keyboard_ready = 1;
					current_keyboard_led = 0;
				}
			}
			break;
			
		case APPLICATION_DISCONNECT:
			/* Slow blink - no device connected */
			if ((current_time - state_change_time) >= pdMS_TO_TICKS(500))
			{
				led_toggle_status();
				state_change_time = current_time;
				if (++blink_count > 10)
				{
					DBG_I("Waiting for USB HID Keyboard connection\r\n");
					blink_count = 0;
				}
			}
			break;
			
		default:
			/* Fast blink - device not supported or other states */
			if ((current_time - state_change_time) >= pdMS_TO_TICKS(100))
			{
				led_toggle_status();
				state_change_time = current_time;
				if (++blink_count > 10)
				{
					DBG_I("USB device not supported or in transition\r\n");
					blink_count = 0;
				}
			}
			break;
	}
}

/**
  * @brief  Process keyboard data and send to Amiga task
  * @details Reads keyboard data from USB HID interface and forwards it to the Amiga task
  *          via the keyboard_queue. Handles keyboard scan code acquisition and message
  *          packaging with timestamp information.
  * @param  None
  * @retval None
  * @note   Only processes data when keyboard is ready and USB host is available
  *         Uses non-blocking queue send to avoid task blocking
  */
static void usb_task_process_keyboard(void)
{
	if (usbhost == NULL) return;
	
	/* Get keyboard data */
	if (USBH_Keybd(usbhost) == 0)
	{
		keyboard_code_t *scancode = USBH_GetScanCode();
		if (scancode != NULL)
		{
			/* Prepare message for Amiga task */
			keyboard_message_t msg;
			msg.keycode = *scancode;
			msg.timestamp = xTaskGetTickCount();
			
			/* Send to Amiga task */
			if (xQueueSend(keyboard_queue, &msg, 0) != pdTRUE)
			{
				DBG_W("Failed to send keyboard data to Amiga task\r\n");
			}
			else
			{
				DBG_V("Keyboard data sent to Amiga task\r\n");
			}
		}
	}
}

/**
  * @brief  Handle LED messages from Amiga task
  * @details Processes LED control messages received from the Amiga task via led_queue.
  *          Updates keyboard LED state based on Amiga keyboard status (Caps Lock, Num Lock,
  *          Scroll Lock) and handles special reset blink sequence.
  * @param  None
  * @retval None
  * @note   Processes all available messages in queue during each call
  *         Maintains current LED state in current_keyboard_led variable
  */
static void usb_task_handle_led_messages(void)
{
	led_message_t led_msg;
	
	/* Check for LED messages from Amiga task */
	while (xQueueReceive(led_queue, &led_msg, 0) == pdTRUE)
	{
		DBG_V("Received LED message: %d\r\n", led_msg.led_status);
		
		switch (led_msg.led_status)
		{
			case LED_CAPS_LOCK_OFF:
				current_keyboard_led &= ~CAPS_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_CAPS_LOCK_ON:
				current_keyboard_led |= CAPS_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_NUM_LOCK_OFF:
				current_keyboard_led &= ~NUM_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_NUM_LOCK_ON:
				current_keyboard_led |= NUM_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_SCROLL_LOCK_OFF:
				current_keyboard_led &= ~SCROLL_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_SCROLL_LOCK_ON:
				current_keyboard_led |= SCROLL_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_RESET_BLINK:
				DBG_I("Reset occurred - reinitializing keyboard LEDs\r\n");
				usb_keyboard_led_init_sequence(usbhost);
				current_keyboard_led = 0;
				break;
				
			default:
				DBG_V("No LED action required\r\n");
				break;
		}
	}
}

/**
  * @brief  Set keyboard LED state
  * @details Sends HID report to set the state of keyboard LEDs (Caps Lock, Num Lock, Scroll Lock).
  *          Implements retry mechanism for reliable LED control with configurable retry count.
  * @param  usbhost: Pointer to USB Host handle
  * @param  led: LED state bitmask (combination of CAPS_LOCK_LED, NUM_LOCK_LED, SCROLL_LOCK_LED)
  * @retval None
  * @note   Uses USBH_HID_SetReport with report type 0x02 (Output Report)
  *         Includes retry mechanism with USB_REPORT_RETRY attempts
  */
void usb_keyboard_led_set(USBH_HandleTypeDef *usbhost, keyboard_led_t led)
{
	if (usbhost == NULL) return;
	
	USBH_StatusTypeDef status;
	int retry_count = USB_REPORT_RETRY;
	
	DBG_N("Setting USB keyboard LED: 0x%02x\r\n", led);
	
	for (int i = 0; i < retry_count; i++)
	{
		status = USBH_HID_SetReport(usbhost, 0x02, 0x00, &led, 1);
		DBG_N("[%d] USB LED Status: %d\r\n", retry_count - i, status);
		
		if (status == USBH_OK)
		{
			break;
		}
		
		vTaskDelay(pdMS_TO_TICKS(1)); // Small delay between retries
	}
}

/**
  * @brief  Initialize keyboard LEDs with blink sequence
  * @details Performs keyboard LED initialization sequence by blinking all LEDs twice
  *          to indicate successful keyboard connection and initialization. This provides
  *          visual feedback to the user that the keyboard is ready for operation.
  * @param  usbhost: Pointer to USB Host handle
  * @retval None
  * @note   Sequence: All LEDs ON (250ms) -> OFF (125ms) -> ON (250ms) -> OFF (final)
  *         Total sequence duration: approximately 750ms
  */
void usb_keyboard_led_init_sequence(USBH_HandleTypeDef *usbhost)
{
	if (usbhost == NULL) return;
	
	keyboard_led_t all_leds = CAPS_LOCK_LED | NUM_LOCK_LED | SCROLL_LOCK_LED;
	
	DBG_N("Starting keyboard LED initialization sequence\r\n");
	
	for (int i = 0; i < 2; i++)
	{
		DBG_V("LEDs ON\r\n");
		usb_keyboard_led_set(usbhost, all_leds);
		vTaskDelay(pdMS_TO_TICKS(250));
		
		DBG_V("LEDs OFF\r\n");
		usb_keyboard_led_set(usbhost, 0);
		vTaskDelay(pdMS_TO_TICKS(125));
	}
	
	/* Final state: all LEDs off */
	usb_keyboard_led_set(usbhost, 0);
	DBG_N("Keyboard LED initialization sequence complete\r\n");
}

/**
  * @brief  Get USB task state
  * @details Returns the current state of the USB task for monitoring and debugging purposes.
  * @param  None
  * @retval task_state_t: Current task state (TASK_STATE_INIT, TASK_STATE_RUNNING, etc.)
  * @note   Used for task health monitoring and system diagnostics
  */
task_state_t usb_task_get_state(void)
{
	return usb_task_state;
}

/**
  * @brief  Toggle status LED
  * @details Toggles the status LED (TP1) to provide visual indication of USB task activity
  *          and system state. Used for different blinking patterns based on USB connection status.
  * @param  None
  * @retval None
  * @note   Controls TP1_Pin on TP1_GPIO_Port
  *         LED state is maintained in static variable for toggle operation
  */
static void led_toggle_status(void)
{
	static int led_state = 0;
	
	led_state = !led_state;
	HAL_GPIO_WritePin(TP1_GPIO_Port, TP1_Pin, led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

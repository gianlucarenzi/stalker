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
#include "amiga.h"
#include "hid_report.h"
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
static volatile int usb_initialized = 0;

/** @brief Flag indicating if keyboard is ready for operation */
static volatile int keyboard_ready = 0;

/** @brief Current keyboard LED state bitmask */
static keyboard_led_t current_keyboard_led = 0;

/* Private function prototypes -----------------------------------------------*/
static void usb_task_process_keyboard(void);
static void usb_task_handle_led_messages(void);
void usb_task_send_string(const char *str);

/* Private functions ---------------------------------------------------------*/
static void led_light(int state)
{
	int tpval = GPIO_PIN_RESET;

	if (!!state)
	{
		tpval = GPIO_PIN_SET;
	}
	else
	{
		tpval = GPIO_PIN_RESET;
	}
	HAL_GPIO_WritePin(TP1_GPIO_Port, TP1_Pin, tpval);
}

void led_toggle(void)
{
	static int tpval = 0;
	if (tpval == 0)
	{
		tpval = 1;
	}
	else
	{
		tpval = 0;
	}
	led_light(tpval);
}

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
	TickType_t g_timer = 0;
	const TickType_t task_frequency = pdMS_TO_TICKS(10); // 10ms cycle
	int timerOneShot = 1;
	TickType_t current_time;
	int count = 0;

	DBG_W("USB Task started\r\n");
	
	usb_task_state = TASK_STATE_RUNNING;
	
	/* Initialize the task */
	usb_task_init();
	
	/* Initialize the xLastWakeTime variable with the current time */
	last_wake_time = xTaskGetTickCount();
	
	for (;;)
	{
		/* Handle USB Host processing */
		if (!usb_initialized)
		{
			DBG_I("MX_USB_HOST_Init()\r\n");
			/* Initialize USB HOST OTG FS */
			MX_USB_HOST_Init();
			usb_initialized = ! usb_initialized;
		}

		MX_USB_HOST_Process();
		usb_app_state = USBH_ApplicationState();

		// Se risulta connessa la tastiera USB
		if (usb_app_state == APPLICATION_READY)
		{
			DBG_N("APPLICATION READY\n\r");
			usbhost = USBH_GetHost();
			if (usbhost != NULL)
			{
				// Controlliamo che abbiamo collegato una tastiera
				if (USBH_HID_GetDeviceType(usbhost) == HID_KEYBOARD)
				{
					if ( !timerOneShot )
					{
						timerOneShot = !timerOneShot;
						DBG_I("#### KEYBOARD CONNECTED ####\r\n");
						g_timer = xTaskGetTickCount();
					}

					if ( !keyboard_ready )
					{
						DBG_N("### BOARD LED ON ### WAIT 500msec FOR LEDS\r\n");
						/* Info: HID protocol was requested to REPORT at class activation.
						 * If device doesn't support it, library keeps BOOT protocol.
						 * Future: here we can query and parse report descriptors. */
						led_light(0);
						current_time = xTaskGetTickCount() - g_timer;
						if (current_time >= pdMS_TO_TICKS(500))
						{
							DBG_I("### KEYBOARD LED TOGGLE ###\r\n");
							/* HOOK: request and parse HID report descriptor for extended keys */
							(void)hid_report_init_for_interface(usbhost, 0);
							if (hid_report_parse_descriptor(usbhost, 0) != 0) {
								DBG_W("HID report descriptor parsing failed or not supported\r\n");
							}
							usb_keyboard_led_init_sequence(usbhost);
							keyboard_ready = 1;
							current_keyboard_led = 0;
						}
					}

					// Get data from keyboard
					if (USBH_Keybd(usbhost) == 0)
					{
						DBG_N("HAVE A KEY EVENT\r\n");
						// Send the keypress to Amiga Task
						usb_task_process_keyboard();
						/* Best-effort attempt to fetch an extended INPUT report (Report Protocol)
						 * Note: size capped; decoding is minimal/log-only for now. */
						{
							uint8_t raw[32];
							if (USBH_HID_GetReport(usbhost, 0x01 /* INPUT */, 0 /* any */, raw, sizeof(raw)) == USBH_OK)
							{
								/* Provide a small callback to enqueue the event */
								void enqueue_cb(const hid_input_event_t *evt) {
									if (extended_input_queue) {
										( void ) xQueueSend(extended_input_queue, evt, 0);
									}
								}
								(void)hid_report_decode(usbhost, 0, raw, sizeof(raw), enqueue_cb);
							}
						}
					}
					else
					{
						// In IDLE mode, check if there are some
						// RESET request on the CLOCK line.
						// Any EXTERNAL Amiga keyboard will assert low
						// the clock line for more than 500msec to
						// obtain the SYSTEM RESET REQUEST, so do we.

						// All Amiga RESET Logic is done into Amiga Task
						// so here we do not need anything more...
						DBG_N("No data from USB Keyboard\r\n");
					}
				}
				else
				{
					// No valid keyboard
					keyboard_ready = 0;
					// Quick blink on device-not-supported
					DBG_I("#### HID Device NOT SUPPORTED ####\r\n");
					if (!timerOneShot)
					{
						timerOneShot = !timerOneShot;
						g_timer = xTaskGetTickCount();
					}

					current_time = xTaskGetTickCount() - g_timer;
					if (current_time >= pdMS_TO_TICKS(100))
					{
						DBG_E("UNKNOWN USB DEVICE count: %d\r\n", count);
						led_toggle();
						g_timer = xTaskGetTickCount();
						if (count++ > 10)
						{
							#ifdef __EASTER_EGG__
							DBG_N("EASTER EGG SENDING STRING:\r\n\tNOT USB Keyboard, but HID Compliant. Please Connect a real USB HID Keyboard!\n");
							#endif
							DBG_N("Waiting a REAL USB Keyboard - Amiga Is Back!\r\n");
							count = 0;
						}
					}
				}
			}
			else
			{
				// Pretty Quick blink on no-hid device plugged
				DBG_I("NO HID DEVICE FOUND.\r\n");
				keyboard_ready = 0;
				if (!timerOneShot)
				{
					timerOneShot = !timerOneShot;
					g_timer = xTaskGetTickCount();
				}
				current_time = xTaskGetTickCount() - g_timer;
				if (current_time >= pdMS_TO_TICKS(250))
				{
					DBG_N("NO HID DEVICE FOUND\r\n");
					led_toggle();
					g_timer = xTaskGetTickCount();
					if (count++ > 10)
					{
						#ifdef __EASTER_EGG__
						usb_task_send_string("NO USB Found Keyboard Device. Please Connect - Amiga Is Back!\n");
						#endif
						DBG_I("Waiting USB HID Keyboard!\r\nPlease Connect\r\n");
						count = 0;
					}
				}
			}
		}
		else
		{
			// We need to manage:
			//
			// APPLICATION_START
			//
			// APPLICATION_DISCONNECT
			//
			// APPLICATION IDLE
			switch( usb_app_state )
			{
				case APPLICATION_START:
					DBG_N("APPLICATION START\r\n");
					vTaskDelay(10);
					break;
				case APPLICATION_DISCONNECT:
					DBG_N("APPLICATION DISCONNECT\r\n");
					break;
				case APPLICATION_IDLE:
					DBG_N("APPLICATION IDLE\r\n");
				default:
					break;
			}
			keyboard_ready = 0;
			// On first run, we start a timer and every 1/2 second we
			// toggle LED Pin
			if (timerOneShot)
			{
				DBG_E("UNCONNECTED USB HID KEYBOARD. PLEASE CONNECT\r\n");
				g_timer = xTaskGetTickCount();
				timerOneShot = 0;
			}
			// slow blink on no device connected
			current_time = xTaskGetTickCount() - g_timer;
			if (current_time >= pdMS_TO_TICKS(500))
			{
				DBG_N("WAIT INSERT USB KEYBOARD count: %d\r\n", count);
				led_toggle();
				g_timer = xTaskGetTickCount();
				if (count++ > 10)
				{
					#ifdef __EASTER_EGG__
					usb_task_send_string("NO USB Keyboard Device Connected. Please Connect! Amiga Is Back!\n");
					#endif
					DBG_I("Waiting USB HID Keyboard!\r\nPlease Connect\r\n");
					count = 0;
				}
			}
		}
		/* Reading from the led status from Amiga Side is always possible */
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
	
//	/* Initialize USB HOST OTG FS */
//	MX_USB_HOST_Init();
//	usb_initialized = 1;
	
	DBG_N("USB Task initialization complete\r\n");
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
	if (usbhost == NULL)
	{
		DBG_E("No usbhost ready yet.\r\n");
		return;
	}

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
	else
	{
		DBG_E("No ScanCode received. Weird?\r\n");
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
				DBG_V("CAPS LOCK LED OFF\r\n");
				current_keyboard_led &= ~CAPS_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_CAPS_LOCK_ON:
				DBG_V("CAPS LOCK LED ON\r\n");
				current_keyboard_led |= CAPS_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_NUM_LOCK_OFF:
				DBG_V("NUM LOCK LED OFF\r\n");
				current_keyboard_led &= ~NUM_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_NUM_LOCK_ON:
				DBG_V("NUM LOCK LED ON\r\n");
				current_keyboard_led |= NUM_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_SCROLL_LOCK_OFF:
				DBG_V("SCROLL LOCK LED OFF\r\n");
				current_keyboard_led &= ~SCROLL_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_SCROLL_LOCK_ON:
				DBG_V("SCROLL LOCK LED ON\r\n");
				current_keyboard_led |= SCROLL_LOCK_LED;
				usb_keyboard_led_set(usbhost, current_keyboard_led);
				break;
				
			case LED_RESET_BLINK:
				DBG_I("Reset occurred from Amiga Side - reinitializing keyboard LEDs\r\n");
				usb_keyboard_led_init_sequence(usbhost);
				current_keyboard_led = 0;
				break;
				
			case NO_LED:
			default:
				break;
		}
	}
}

/**
  * @brief  Send a string to the Amiga keyboard interface
  * @details This function takes a string, converts each character to Amiga scancodes,
  *          and sends them as key press/release events to the Amiga via amikb_notify.
  * @param  str: The string to be sent.
  * @retval None
  */
void usb_task_send_string(const char *str)
{
	// TO BE IMPLEMENTED!
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
	
	DBG_W("Setting USB keyboard LED: 0x%02x\r\n", led);
	
	for (int i = 0; i < retry_count; i++)
	{
		status = USBH_HID_SetReport(usbhost, 0x02, 0x00, &led, 1);
		DBG_N("[%d] USB LED Status: %d\r\n", retry_count - i, status);
		
		if (status == USBH_OK)
		{
			break;
		}
		
		vTaskDelay(pdMS_TO_TICKS(10)); // Small delay between retries
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

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_task.c
  * @brief          : USB Keyboard Task Implementation
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
#include "usb_task.h"
#include "usb_host.h"
#include "main.h"
#include "cmsis_os.h"
#include "usbh_hid.h"
#include "usbh_hid_keybd.h"
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "usb_hid_keys.h"
#include "led_manager_task.h"
#include "amiga.h" // Added include for amiga functions

/* External variables --------------------------------------------------------*/
extern osMessageQueueId_t keyboardQueueHandle;
extern osMessageQueueId_t ledQueueHandle;
extern osMessageQueueId_t ledManagerQueueHandle;
extern USBH_HandleTypeDef hUsbHostFS;

/* Private variables ---------------------------------------------------------*/
static ApplicationTypeDef previous_state = APPLICATION_IDLE;
static uint32_t last_led_toggle_time = 0;
static uint8_t led_blink_state = 0;
static uint8_t usb_led_state = 0;  /**< Track USB LED state to avoid flooding queue */
static uint8_t previous_keys[6] = {0};     /**< Previous HID keys array for change detection */
static uint8_t previous_modifiers = 0;     /**< Previous modifier keys for change detection */
static keyboard_led_t current_led_state = 0;  /**< Current keyboard LED state bitmap */
static keyboard_led_t pending_led_state = 0;  /**< Pending LED state to send (for retry) */
static uint8_t led_update_pending = 0;     /**< Flag: 1 if LED update is pending */
static uint16_t led_retry_count = 0;       /**< Number of retry attempts for current LED update */
static uint16_t led_max_retries_seen = 0;  /**< Maximum retry count observed */

#define LED_MAX_RETRY_THRESHOLD 20         /**< Maximum retry attempts before giving up */

/* Private variables for reset blink sequence */
static uint8_t reset_blink_active = 0;
static uint8_t reset_blink_count = 0;
static uint32_t last_reset_blink_time = 0;
#define RESET_BLINK_CYCLES 6 /* 3 ON/OFF cycles */
#define RESET_BLINK_INTERVAL_MS 250


/* Private function prototypes -----------------------------------------------*/
static void led_on(void);
static void led_toggle(void);
static void handle_usb_state(void);
static void handle_keyboard_input(void);
static void send_reset_event(void);
static const char* get_state_name(ApplicationTypeDef state);
static void handle_reset_blink_sequence(void);
static void handle_led_messages(void);
static void try_send_pending_led_update(void);
static void usb_keyboard_led_set(USBH_HandleTypeDef *usbhost, keyboard_led_t led);
static ApplicationTypeDef get_application_state(void);


static int debuglevel = DBG_INFO;

/**
  * @brief  Sends a string by simulating key presses and releases on the Amiga side.
  * @param  str: The string to send.
  * @retval None
  */
void usb_task_send_string(const char *str)
{
	uint8_t ascii_char;
	uint8_t usb_hid_scancode;
	uint8_t amiga_scancode;

	if (str == NULL)
	{
		return;
	}

	DBG_W("Sending string: \"%s\"\r\n", str);

	for (int i = 0; str[i] != '\0'; i++)
	{
		ascii_char = (uint8_t)str[i];
		usb_hid_scancode = ascii_to_scancode(ascii_char);
		amiga_scancode = scancode_to_amiga(usb_hid_scancode);

		// Simulate key press
		// Assuming amikb_send uses osDelay internally if use_OS is 1
		amikb_send(amiga_scancode, 1, 1); // 1 for press, 1 for use_OS (FreeRTOS delay)
		osDelay(60); // Small delay between press and release

		// Simulate key release
		amikb_send(amiga_scancode, 0, 1); // 0 for release, 1 for use_OS
		osDelay(60); // Small delay between characters
	}
}

/* Task Implementation -------------------------------------------------------*/

/**
  * @brief  Function implementing the USB Keyboard Task thread.
  * @param  argument: Not used
  * @retval None
  */
void usbKeyboardTask(void *argument)
{
	DBG_N("Called\r\n");

	/* init code for USB_HOST */
	MX_USB_HOST_Init();

	/* Infinite loop */
	DBG_N("%s Now loop\r\n", __PRETTY_FUNCTION__);

	static uint32_t last_easter_egg_send_time = 0;
	const uint32_t easter_egg_interval_ms = 15000; // 15 seconds

	for(;;)
	{
		handle_usb_state();
		handle_keyboard_input();
		handle_reset_blink_sequence();
		handle_led_messages();
		try_send_pending_led_update();

		// Easter Egg Logic
#if defined(__EASTER_EGG__) || defined(__AMIBERRY_EASTER_EGG__)
		if (get_application_state() != APPLICATION_READY)
		{
			uint32_t current_time = osKernelGetTickCount();
			if ((current_time - last_easter_egg_send_time) >= easter_egg_interval_ms)
			{
				const char *easter_egg_string = NULL;
#if defined(__AMIBERRY_EASTER_EGG__)
				easter_egg_string = "AmiBerry Dimitris Panokostas VERSION. Let's emulate Amiga!";
#elif defined(__EASTER_EGG__)
				easter_egg_string = "Please Connect USB Keyboard to your Amiga! Amiga is Back!";
#endif
				if (easter_egg_string != NULL)
				{
					usb_task_send_string(easter_egg_string);
				}
				last_easter_egg_send_time = current_time;
			}
		}
#endif // defined(__EASTER_EGG__) || defined(__AMIBERRY_EASTER_EGG__)

		osDelay(10);  // Check state every 10ms for responsive LED blinking and keyboard polling
	}
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Get human-readable name for application state
  * @param  state: Application state
  * @retval State name string
  */
static const char* get_state_name(ApplicationTypeDef state)
{
	switch(state)
	{
		case APPLICATION_IDLE:
			return "IDLE";
		case APPLICATION_START:
			return "START";
		case APPLICATION_READY:
			return "READY";
		case APPLICATION_DISCONNECT:
			return "DISCONNECT";
		default:
			return "UNKNOWN";
	}
}

/**
  * @brief  Handle keyboard input reading and queuing (RAW data without filtering)
  * @retval None
  */
static void handle_keyboard_input(void)
{
	// Only read keyboard when device is ready and it's a keyboard
	if (get_application_state() != APPLICATION_READY)
	{
		return;
	}

	// Check if connected device is a keyboard
	if (USBH_HID_GetDeviceType(&hUsbHostFS) != HID_KEYBOARD)
	{
		return;
	}

	// Get keyboard info
	HID_KEYBD_Info_TypeDef *keybd_info = USBH_HID_GetKeybdInfo(&hUsbHostFS);
	if (keybd_info == NULL)
	{
		return;
	}

	// Build modifier byte from individual flags
	uint8_t modifiers = 0;
	if (keybd_info->lctrl)  modifiers |= 0x01;
	if (keybd_info->lshift) modifiers |= 0x02;
	if (keybd_info->lalt)   modifiers |= 0x04;
	if (keybd_info->lgui)   modifiers |= 0x08;
	if (keybd_info->rctrl)  modifiers |= 0x10;
	if (keybd_info->rshift) modifiers |= 0x20;
	if (keybd_info->ralt)   modifiers |= 0x40;
	if (keybd_info->rgui)   modifiers |= 0x80;

	// Check if state has changed (compare with previous state)
	uint8_t state_changed = 0;

	// Check if modifiers changed
	if (modifiers != previous_modifiers)
	{
		state_changed = 1;
	}

	// Check if any key changed
	if (!state_changed)
	{
		if (memcmp(keybd_info->keys, previous_keys, 6) != 0)
		{
			state_changed = 1;
		}
	}

	// Only send message if state has changed
	if (state_changed)
	{
		// Debug: print raw keys received
		DBG_V("Raw HID Report: mod=0x%02X keys=[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]\r\n",
		      modifiers,
		      keybd_info->keys[0], keybd_info->keys[1], keybd_info->keys[2],
		      keybd_info->keys[3], keybd_info->keys[4], keybd_info->keys[5]);

		// Build keyboard message
		keyboard_message_t msg;
		msg.timestamp = HAL_GetTick();
		msg.is_reset_event = 0;
		msg.modifiers = modifiers;

		// Copy keys directly
		for (uint8_t i = 0; i < 6; i++)
		{
			msg.keys[i] = keybd_info->keys[i];
		}

		// Send single message with complete keyboard state
		osStatus_t status = osMessageQueuePut(keyboardQueueHandle, &msg, 0, 0);
		if (status != osOK)
		{
			DBG_E("keyboardQueueHandle full\r\n");
		}

		// Update previous state
		memcpy(previous_keys, keybd_info->keys, 6);
		previous_modifiers = modifiers;
	}
}

/**
  * @brief  Send reset event to Amiga task
  * @retval None
  */
static void send_reset_event(void)
{
	keyboard_message_t msg;
	memset(msg.keys, 0, sizeof(msg.keys));
	msg.modifiers = 0;
	msg.timestamp = HAL_GetTick();
	msg.is_reset_event = 1;  // This is a reset event

	osStatus_t status = osMessageQueuePut(keyboardQueueHandle, &msg, 0, 0);
	if (status == osOK)
	{
		DBG_E("Reset event sent to Amiga task\r\n");
	}
}

/**
  * @brief  Handle USB state changes and LED control
  * @retval None
  */
static void handle_usb_state(void)
{
	ApplicationTypeDef current_state = get_application_state();

	// Detect state change and print only once
	if (current_state != previous_state)
	{
		DBG_V("USB State: %s -> %s\r\n",
			   get_state_name(previous_state),
			   get_state_name(current_state));

		// If we just connected the keyboard, trigger a welcome blink
		if (current_state == APPLICATION_READY && previous_state != APPLICATION_READY)
		{
			DBG_V("USB keyboard connected, triggering welcome blink\r\n");
			led_message_t msg;
			msg.led_status = LED_RESET_BLINK;
			msg.timestamp = osKernelGetTickCount();
			osMessageQueuePut(ledQueueHandle, &msg, 0, 0);
		}
		// Send reset event on disconnect or when leaving READY state
		else if (current_state == APPLICATION_DISCONNECT ||
		    current_state == APPLICATION_IDLE ||
		    (previous_state == APPLICATION_READY && current_state != APPLICATION_READY))
		{
			send_reset_event();

			// Reset keyboard state tracking
			memset(previous_keys, 0, sizeof(previous_keys));
			previous_modifiers = 0;

			// Reset LED state
			current_led_state = 0;
		}

		previous_state = current_state;

		// Reset LED state on transition
		led_blink_state = 0;
		last_led_toggle_time = HAL_GetTick();
		usb_led_state = 0;  // Force LED update on state change
	}

	// LED management based on current state
	if (current_state == APPLICATION_READY)
	{
		// Keyboard recognized and working - LED solid ON
		led_on();
	}
	else
	{
		// Any other state - LED blinking at 250ms (2Hz, 50% duty cycle)
		uint32_t current_time = HAL_GetTick();

		if ((current_time - last_led_toggle_time) >= 250)
		{
			led_toggle();
			last_led_toggle_time = current_time;
		}
	}
}

/**
  * @brief  Turn on the LED (only sends message if state changed)
  * @retval None
  */
static void led_on(void)
{
	// Only send message if LED was not already on
	if (usb_led_state != 1)
	{
		led_manager_message_t msg = {
			.command = LED_CMD_ON,
			.data = 0,
			.duration_ms = 0
		};
		osMessageQueuePut(ledManagerQueueHandle, &msg, 0, 0);
		usb_led_state = 1;
	}
}

/**
  * @brief  Toggle the LED state
  * @retval None
  */
static void led_toggle(void)
{
	led_manager_message_t msg = {
		.command = LED_CMD_TOGGLE,
		.data = 0,
		.duration_ms = 0
	};
	osMessageQueuePut(ledManagerQueueHandle, &msg, 0, 0);

	// Update local state for tracking
	led_blink_state = !led_blink_state;
	usb_led_state = 2;  // Mark as blinking
}

/**
  * @brief  Handle the non-blocking reset blink sequence.
  * @retval None
  */
static void handle_reset_blink_sequence(void)
{
	uint32_t current_time;

	if (!reset_blink_active)
	{
		return;
	}

	// On the first activation (count is 0), set the initial state
	if (reset_blink_count == 0)
	{
		DBG_N("Starting LED reset blink sequence: %ld\r\n", osKernelGetTickCount());
		current_led_state = CAPS_LOCK_LED | NUM_LOCK_LED | SCROLL_LOCK_LED; // Start with ON
		pending_led_state = current_led_state;
		led_update_pending = 1;
		led_retry_count = 0;
		reset_blink_count++; // Immediately advance to the first timed state
		last_reset_blink_time = osKernelGetTickCount();
		return; // Allow the first state to be sent
	}

	current_time = osKernelGetTickCount();
	if ((current_time - last_reset_blink_time) >= RESET_BLINK_INTERVAL_MS)
	{
		last_reset_blink_time = current_time;
		reset_blink_count++;

		if (reset_blink_count >= RESET_BLINK_CYCLES)
		{
			DBG_N("Finished LED reset blink sequence: %ld\r\n", osKernelGetTickCount());
			// Blink sequence finished
			reset_blink_active = 0;
			// Restore the correct LED state (all off after reset)
			current_led_state = 0;
		}
		else
		{
			// Toggle all LEDs
			if (current_led_state != 0)
			{
				current_led_state = 0; // Turn off
			}
			else
			{
				current_led_state = CAPS_LOCK_LED | NUM_LOCK_LED | SCROLL_LOCK_LED; // Turn on
			}
		}

		// Set as pending update
		pending_led_state = current_led_state;
		led_update_pending = 1;
		led_retry_count = 0;
	}
}

/**
  * @brief  Handle LED messages from Amiga task
  * @retval None
  */
static void handle_led_messages(void)
{
	led_message_t msg;
	osStatus_t status;

	// We only process one message per task loop iteration
	status = osMessageQueueGet(ledQueueHandle, &msg, NULL, 0);

	if (status == osOK)
	{
		DBG_V("Received LED message: %d\r\n", msg.led_status);

		// If a reset is requested, start it regardless.
		if (msg.led_status == LED_RESET_BLINK) {
			reset_blink_active = 1;
			reset_blink_count = 0;
			// The blink sequence function will handle the LED state
			return; // Exit to let the blink handler take over
		}

		// If a blink is active, ignore all other messages.
		if (reset_blink_active) {
			DBG_W("Ignoring LED message %d during reset blink\r\n", msg.led_status);
			return;
		}

		// Update LED state based on message (normal operation)
		switch (msg.led_status)
		{
			case LED_CAPS_LOCK_ON:
				current_led_state |= CAPS_LOCK_LED;
				break;

			case LED_CAPS_LOCK_OFF:
				current_led_state &= ~CAPS_LOCK_LED;
				break;

			case LED_NUM_LOCK_ON:
				current_led_state |= NUM_LOCK_LED;
				break;

			case LED_NUM_LOCK_OFF:
				current_led_state &= ~NUM_LOCK_LED;
				break;

			case LED_SCROLL_LOCK_ON:
				current_led_state |= SCROLL_LOCK_LED;
				break;

			case LED_SCROLL_LOCK_OFF:
				current_led_state &= ~SCROLL_LOCK_LED;
				break;

			case LED_RESET_BLINK:
				// This case is handled above, should not be reached
				break;

			default:
				DBG_E("Unknown LED status: %d\r\n", msg.led_status);
				return;
		}

		// Debug: print LED state before sending
		DBG_N("LED state update: 0x%02X (N:%d C:%d S:%d)\r\n",
		       current_led_state,
		       (current_led_state & NUM_LOCK_LED) ? 1 : 0,
		       (current_led_state & CAPS_LOCK_LED) ? 1 : 0,
		       (current_led_state & SCROLL_LOCK_LED) ? 1 : 0);

		// Mark LED update as pending for retry mechanism
		pending_led_state = current_led_state;
		led_update_pending = 1;
		led_retry_count = 0;  // Reset retry counter for new update
	}
}

/**
  * @brief  Try to send pending LED update (with retry on BUSY)
  * @retval None
  */
static void try_send_pending_led_update(void)
{
	// Check if there's a pending LED update
	if (!led_update_pending)
	{
		return;
	}

	// Try to send the LED state
	usb_keyboard_led_set(&hUsbHostFS, pending_led_state);
}

/**
  * @brief  Set keyboard LED state via USB
  * @param  usbhost: USB Host handle
  * @param  led: LED bitmap to set (keyboard_led_t)
  * @retval None
  */
static void usb_keyboard_led_set(USBH_HandleTypeDef *usbhost, keyboard_led_t led)
{
	// Only send if keyboard is ready
	if (get_application_state() != APPLICATION_READY)
	{
		return;
	}

	// Check if connected device is a keyboard
	if (USBH_HID_GetDeviceType(usbhost) != HID_KEYBOARD)
	{
		return;
	}

	// Check USB host state
	if (usbhost->gState != HOST_CLASS)
	{
		return;
	}

	// Send HID Output Report to set keyboard LEDs
	// USB HID specification: Report Type 2 = Output Report
	// LED bitmap: bit 0=NUM_LOCK, bit 1=CAPS_LOCK, bit 2=SCROLL_LOCK
	uint8_t led_report = (uint8_t)led;

	USBH_StatusTypeDef status = USBH_HID_SetReport(usbhost,
	                                                2,              // reportType: Output
	                                                0,              // reportId: 0
	                                                &led_report,    // buffer
	                                                1);             // length

	if (status == USBH_OK)
	{
		// Success - clear pending flag
		led_update_pending = 0;

		// Update max retries if this was higher
		if (led_retry_count > led_max_retries_seen)
		{
			led_max_retries_seen = led_retry_count;
		}

		DBG_N("LED OK: 0x%02X (retries:%u, max:%u)\r\n", led, led_retry_count, led_max_retries_seen);
		led_retry_count = 0;
	}
	else if (status == USBH_BUSY)
	{
		// Busy - increment retry counter
		led_retry_count++;

		// Check if we've exceeded the retry threshold
		if (led_retry_count >= LED_MAX_RETRY_THRESHOLD)
		{
			// Give up after too many retries
			led_update_pending = 0;
			DBG_E("LED TIMEOUT: gave up after %u retries (threshold:%d)\r\n",
			       led_retry_count, LED_MAX_RETRY_THRESHOLD);
			led_retry_count = 0;
		}
		// Otherwise will retry next time (silent)
	}
	else
	{
		// Error - clear pending flag and report
		led_update_pending = 0;
		DBG_E("LED ERR: %d (after %u retries)\r\r\n", status, led_retry_count);
		led_retry_count = 0;
	}
}

/**
  * @brief  Get current application state from USB Host
  * @details Thread-safe wrapper around USBH_GetApplicationState
  * @retval ApplicationTypeDef Current application state
  */
static ApplicationTypeDef get_application_state(void)
{
	return USBH_GetApplicationState(&hUsbHostFS);
}

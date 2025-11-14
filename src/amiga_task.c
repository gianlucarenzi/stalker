/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : amiga_task.c
  * @brief          : Amiga Keyboard Task Implementation
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
#include "amiga_task.h"
#include "usb_task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "log_task.h"
#include "amiga.h"
#include "eeprom_task.h"
#include "usb_hid_keys.h"

/* External variables --------------------------------------------------------*/
extern osMessageQueueId_t keyboardQueueHandle;
extern osMessageQueueId_t ledQueueHandle;
extern osMessageQueueId_t amigaTaskQueueHandle;
extern volatile reset_keypress_mode_t current_mode;

/* Private variables ---------------------------------------------------------*/
#define MAX_TRACKED_KEYS 16
static uint8_t pressed_keys[MAX_TRACKED_KEYS] = {0};  /**< Currently pressed keys */
static uint8_t num_pressed_keys = 0;                   /**< Number of currently pressed keys */
static uint8_t previous_keys[6] = {0};                 /**< Previous HID keys array */
static uint8_t previous_modifiers = 0;                 /**< Previous modifier keys */

/* LED state tracking */
static uint8_t caps_lock_state = 0;    /**< CAPS LOCK state: 0=OFF, 1=ON */
static uint8_t num_lock_state = 0;     /**< NUM LOCK state: 0=OFF, 1=ON */
static uint8_t scroll_lock_state = 0;  /**< SCROLL LOCK state: 0=OFF, 1=ON */

/* USB HID scancodes for lock keys */
#define SCANCODE_CAPS_LOCK    0x39
#define SCANCODE_NUM_LOCK     0x53
#define SCANCODE_SCROLL_LOCK  0x47

/* Private function prototypes -----------------------------------------------*/
static void process_keyboard_message(keyboard_message_t *msg);
static void reset_key_state(void);
static uint8_t is_key_pressed(uint8_t scancode);
static void add_pressed_key(uint8_t scancode);
static void remove_pressed_key(uint8_t scancode);
static uint8_t is_key_in_array(uint8_t scancode, const uint8_t *keys_array, uint8_t size);
static uint8_t scancode_from_modifier_bit(uint8_t bit_position);
static void handle_key_press(uint8_t scancode, uint8_t modifiers, uint32_t timestamp);
static void handle_key_release(uint8_t scancode, uint8_t modifiers, uint32_t timestamp);
static void send_led_status(led_status_t status, uint32_t timestamp);
static void amiga_task_check_reset_condition(void);
static void check_for_special_combos(void);
static void amiga_task_handle_reset(int from_amiga);
static void convert_message_to_keyboard_code(keyboard_message_t *msg, keyboard_code_t *code);

static int debuglevel = DBG_INFO;
static int amiga_ready = 0; // Amiga protocol is silent if no usb keyboard is ready && no pressed key

/* Reset detection variables */
static uint32_t reset_timer_start = 0;
static uint8_t reset_timer_active = 0;

/* Helper macro to calculate ticks elapsed */
#define TICKS_SINCE(start) (osKernelGetTickCount() - (start))

/* Task Implementation -------------------------------------------------------*/
/**
  * @brief  Initialize Amiga Task
  * @details Performs Amiga task initialization including GPIO setup and Amiga keyboard
  *          protocol initialization. Sets up the hardware interface for communication
  *          with the Amiga computer and initializes the keyboard protocol state machine.
  * @param  None
  * @retval None
  * @note   Called once during task startup before entering main loop
  *         Initializes Amiga interface in "not ready" state until USB keyboard connects
  */
static void amiga_task_init(void)
{
	DBG_N("Amiga Task initialization START\r\n");

	/* Initialize GPIO for Amiga */
	DBG_N("Calling amikb_gpio_init()\r\n");
	amikb_gpio_init();
	DBG_N("amikb_gpio_init() done\r\n");

	/* Start Amiga keyboard protocol */
	DBG_N("Calling amikb_startup(1)\r\n");
	amikb_startup(1);
	DBG_N("amikb_startup(1) done\r\n");

	/* Set Amiga as ready */
	DBG_N("Calling amikb_ready(0)\r\n");
	amikb_ready(0); // Initially not ready until USB keyboard is connected
	DBG_N("amikb_ready(0) done\r\n");

	DBG_N("Amiga Task initialization COMPLETE\r\n");
}

/**
  * @brief  Function implementing the Amiga Task thread.
  * @param  argument: Not used
  * @retval None
  */
void amigaTask(void *argument)
{
	/* USER CODE BEGIN amigaTask */
	keyboard_message_t msg;
	osStatus_t status;

	DBG_N("Called.\r\n");

	// Read the initial mode from EEPROM at startup.
	// Note: EE_OK is 0. This should be in a shared header.
	uint16_t saved_mode;
	if (eeprom_read_variable_sync(VIRTUAL_ADDR_SYSTEM_MODE, &saved_mode) == 0 /* EE_OK */) {
		if (saved_mode == AMIGA_MODE || saved_mode == PC_MODE) {
			current_mode = (reset_keypress_mode_t)saved_mode;
			DBG_I("AMIGA_TASK: Found saved mode. Setting current_mode to %d as %s\r\n", current_mode, current_mode == AMIGA_MODE ? "AMIGA_MODE" : "PC_MODE");
		} else {
			DBG_W("AMIGA_TASK: Corrupted mode found. Using default and saving.\r\n");
			current_mode = AMIGA_MODE;
			send_eeprom_write_request(VIRTUAL_ADDR_SYSTEM_MODE, (uint16_t)current_mode);
		}
	} else {
		// This block is hit if the variable doesn't exist (first boot) or a read error occurred.
		DBG_I("AMIGA_TASK: No saved mode found. Using default and saving.\r\n");
		current_mode = AMIGA_MODE;
		send_eeprom_write_request(VIRTUAL_ADDR_SYSTEM_MODE, (uint16_t)current_mode);
	}

	amiga_task_init();

	DBG_W("Amiga Task: Waiting for keyboard events...\r\n");

	/* Infinite loop */
	for(;;)
	{
		// Check for keyboard message from USB task (non-blocking)
		status = osMessageQueueGet(keyboardQueueHandle, &msg, NULL, 0);

		if (status == osOK)
		{
			// Process the received keyboard message to update logical key state
			process_keyboard_message(&msg);

			// Convert and send to Amiga protocol
			keyboard_code_t code;
			convert_message_to_keyboard_code(&msg, &code);
			amikb_process(&code, 1 /* use_OS */);

			if (!amiga_ready)
			{
				amiga_ready = 1;
				amikb_ready(amiga_ready);
				DBG_I("Amiga Keyboard Interface is now ready\r\n");
			}
		}

		// Check for internal messages for this task
		AmigaTaskMsg_t internal_msg;
		status = osMessageQueueGet(amigaTaskQueueHandle, &internal_msg, NULL, 0);

		if (status == osOK)
		{
			if (internal_msg == AMIGA_TASK_MSG_RESET_START)
			{
				DBG_N("Reset message received, triggering LED blink\r\n");
				send_led_status(LED_RESET_BLINK, osKernelGetTickCount());
			}
		}

		// Check for special key combinations based on logical key state
		check_for_special_combos();

		/* Check for reset conditions from Amiga side */
		amiga_task_check_reset_condition();

		/* Small delay to avoid busy-wait */
		osDelay(10);
	}
	/* USER CODE END amigaTask */
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Checks for special keyboard combinations (reset, mode toggle).
  * @details This function checks the logically pressed keys and modifiers to
  *          detect special combinations. It uses state machines to handle
  *          debouncing and long-press timers.
  * @param  None
  * @retval None
  */
static void check_for_special_combos(void)
{
	// --- State for Mode Toggle Combo ---
	static bool mode_toggle_combo_active = false;

	// --- State for Reset Combo ---
	static enum { RESET_IDLE, RESET_ARMED, RESET_WAIT_FOR_RELEASE } reset_state = RESET_IDLE;
	static uint32_t reset_timer_start = 0;

	// --- Check current key states ---
	// Modifiers
	bool lctrl_down = (previous_modifiers & MOD_L_CTRL_Msk);
	bool lshift_down = (previous_modifiers & MOD_L_SHIFT_Msk);
	bool lalt_down = (previous_modifiers & MOD_L_ALT_Msk);
	bool rctrl_down = (previous_modifiers & MOD_R_CTRL_Msk);
	bool ralt_down = (previous_modifiers & MOD_R_ALT_Msk);
	bool lgui_down = (previous_modifiers & MOD_L_GUI_Msk);
	bool rgui_down = (previous_modifiers & MOD_R_GUI_Msk);

	// Regular keys
	bool p_down = is_key_pressed(KEY_P);
	bool del_down = is_key_pressed(KEY_DELETE);
	bool app_down = is_key_pressed(KEY_APPLICATION);


	// --- Mode Toggle Logic (LCTRL + LALT + LSHIFT + 'P') ---
	if (lctrl_down && lalt_down && lshift_down && p_down)
	{
		if (!mode_toggle_combo_active)
		{
			mode_toggle_combo_active = true; // Latch until keys are released

			if (current_mode == AMIGA_MODE)
			{
				current_mode = PC_MODE;
				DBG_I("Switched to PC_MODE\r\n");
			}
			else
			{
				current_mode = AMIGA_MODE;
				DBG_I("Switched to AMIGA_MODE\r\n");
			}
			// Notify the EEPROM task to save the new mode
			send_eeprom_write_request(VIRTUAL_ADDR_SYSTEM_MODE, (uint16_t)current_mode);
		}
	}
	else
	{
		// Combo not pressed, release the latch
		mode_toggle_combo_active = false;
	}


	// --- Reset Logic ---
	bool reset_triggered = false;
	if (current_mode == PC_MODE)
	{
		// PC Mode Reset: CTRL + ALT + DEL
		if ((lctrl_down || rctrl_down) && (lalt_down || ralt_down) && del_down)
		{
			reset_triggered = true;
		}
	}
	else // AMIGA_MODE
	{
		// Amiga Mode Reset: LCTRL + LGUI + (RGUI or APP)
		if (lctrl_down && lgui_down && (rgui_down || app_down))
		{
			reset_triggered = true;
		}
	}

	if (reset_triggered)
	{
		if (reset_state == RESET_IDLE)
		{
			// Combo just pressed, arm the timer
			reset_state = RESET_ARMED;
			reset_timer_start = osKernelGetTickCount();
			DBG_I("Reset combination pressed, starting timer...\r\n");
		}
		else if (reset_state == RESET_ARMED) // Combo is still held down, check if timeout is reached
		{
			if (osKernelGetTickCount() - reset_timer_start >= RESET_TIMEOUT_MS)
			{
				DBG_I("#### <SYSTEM RESET> ####\r\n");
				amikb_reset(1); // Perform the hardware reset

				// Notify this task to blink LEDs etc.
				AmigaTaskMsg_t msg = AMIGA_TASK_MSG_RESET_START;
				osMessageQueuePut(amigaTaskQueueHandle, &msg, 0, 0);

				// Transition to a state where we wait for keys to be released
				reset_state = RESET_WAIT_FOR_RELEASE;
			}
		}
		// If reset_state is RESET_WAIT_FOR_RELEASE and reset_triggered is still true, do nothing.
		// We are waiting for the keys to be released.
	}
	else // reset_triggered == false
	{
		if (reset_state == RESET_ARMED)
		{
			// Keys were released before the timeout (glitch)
			DBG_E("Reset combination released before 500ms (glitch).\r\n");
		}
		// In any case, if the combo is not pressed, we are idle.
		// This covers both RESET_ARMED (glitch) and RESET_WAIT_FOR_RELEASE (keys finally released).
		reset_state = RESET_IDLE;
	}
}

/**
  * @brief  Process keyboard message and track key state (RAW HID data)
  * @param  msg: Pointer to keyboard message
  * @retval None
  */
static void process_keyboard_message(keyboard_message_t *msg)
{
	// Check if this is a reset event (disconnect)
	if (msg->is_reset_event)
	{
		DBG_W("[%lu ms] USB DISCONNECT - Resetting key state\r\n", msg->timestamp);
		reset_key_state();
		memset(previous_keys, 0, sizeof(previous_keys));
		previous_modifiers = 0;
		return;
	}

	// Process modifier keys changes
	uint8_t modifier_changes = msg->modifiers ^ previous_modifiers;

	if (modifier_changes != 0)
	{
		// Check each modifier bit
		for (uint8_t bit = 0; bit < 8; bit++)
		{
			uint8_t bit_mask = (1 << bit);

			if (modifier_changes & bit_mask)
			{
				uint8_t scancode = scancode_from_modifier_bit(bit);

				if (msg->modifiers & bit_mask)
				{
					// Modifier pressed
					handle_key_press(scancode, msg->modifiers, msg->timestamp);
				}
				else
				{
					// Modifier released
					handle_key_release(scancode, msg->modifiers, msg->timestamp);
				}
			}
		}
	}

	// Process regular keys changes
	// Check for key releases (keys in previous but not in current)
	for (uint8_t i = 0; i < 6; i++)
	{
		uint8_t prev_key = previous_keys[i];

		if (prev_key != 0)  // Valid key was pressed before
		{
			// Check if this key is still in the current keys array
			if (!is_key_in_array(prev_key, msg->keys, 6))
			{
				// Key was released
				handle_key_release(prev_key, msg->modifiers, msg->timestamp);
			}
		}
	}

	// Check for key presses (keys in current but not in previous)
	for (uint8_t i = 0; i < 6; i++)
	{
		uint8_t curr_key = msg->keys[i];

		if (curr_key != 0)  // Valid key is pressed now
		{
			// Check if this key was in the previous keys array
			if (!is_key_in_array(curr_key, previous_keys, 6))
			{
				// Key was pressed
				handle_key_press(curr_key, msg->modifiers, msg->timestamp);
			}
		}
	}

	// Update previous state
	memcpy(previous_keys, msg->keys, 6);
	previous_modifiers = msg->modifiers;

	// Show currently pressed keys count (only if debug level is high enough)
	DBG_V("  -> Pressed keys: %d\r\n", num_pressed_keys);
}

/**
  * @brief  Handle Amiga reset sequence
  * @details Performs complete Amiga reset sequence including protocol reset and restart.
  *          Sends LED reset blink command to USB task to provide visual feedback of reset
  *          operation. This function is called when reset timeout is reached.
  * @param  int use_OS: if it is true, the routines uses the 
  * @retval None
  * @note   Resets Amiga keyboard protocol state machine
  *         Triggers LED blink sequence on USB keyboard
  *         Restarts Amiga keyboard protocol after reset
  */
static void amiga_task_handle_reset(int use_OS)
{
	DBG_I("Performing Amiga reset sequence\r\n");
	
	/* Perform the reset */
	amikb_reset(1 /* use_OS */);
	
	/* Restart the Amiga keyboard protocol */
	amikb_startup(1 /* use_OS */);
	
	/* Send internal message to trigger LED blink, unifying the logic */
    AmigaTaskMsg_t msg = AMIGA_TASK_MSG_RESET_START;
    osMessageQueuePut(amigaTaskQueueHandle, &msg, 0, 0);

	DBG_I("Amiga reset sequence complete\r\n");
}

/**
  * @brief  Check for reset conditions from Amiga side
  * @details Monitors the Amiga keyboard clock line for reset conditions. External Amiga
  *          keyboards can request system reset by holding the clock line low for more than
  *          500ms. Implements timer-based detection to avoid false triggers from normal
  *          keyboard communication.
  * @param  None
  * @retval None
  * @note   Only checks reset when no keyboard activity is pending in queue
  *         Reset timer is automatically cancelled if clock line goes high
  *         Calls amiga_task_handle_reset() when timeout is reached
  */
static void amiga_task_check_reset_condition(void)
{
	/* Only check reset if we're in idle state (no keyboard activity) */
	if (osMessageQueueGetCount(keyboardQueueHandle) == 0)
	{
		/* Check if CLOCK line is being held low by external Amiga keyboard */
		if (amikb_reset_check())
		{
			/* Clock line is low - start or continue reset timer */
			if (!reset_timer_active)
			{
				reset_timer_start = osKernelGetTickCount();
				reset_timer_active = 1;
				DBG_E("Reset condition detected - starting timer\r\n");
			}
			else
			{
				/* Check if reset timeout has elapsed */
				uint32_t elapsed = TICKS_SINCE(reset_timer_start);
				if (elapsed >= RESET_TIMEOUT_MS)
				{
					DBG_E("Reset timeout elapsed - performing Amiga reset\r\n");
					DBG_I("#### <SYSTEM RESET> from Amiga Hardware ####\r\n");
					amiga_task_handle_reset(1);
					reset_timer_active = 0;
				}
			}
		}
		else
		{
			/* Clock line is high - cancel reset timer */
			if (reset_timer_active)
			{
				reset_timer_active = 0;
				DBG_E("Reset condition cleared\r\n");
			}
		}
	}
}

/**
  * @brief  Reset all tracked key states
  * @retval None
  */
static void reset_key_state(void)
{
	if (num_pressed_keys > 0)
	{
		DBG_W("Releasing all %d pressed keys\r\n", num_pressed_keys);
		memset(pressed_keys, 0, sizeof(pressed_keys));
		num_pressed_keys = 0;
	}

	// Reset LED states
	caps_lock_state = 0;
	num_lock_state = 0;
	scroll_lock_state = 0;
}

/**
  * @brief  Check if a key is currently pressed
  * @param  scancode: USB HID scancode
  * @retval 1 if pressed, 0 if not pressed
  */
static uint8_t is_key_pressed(uint8_t scancode)
{
	for (uint8_t i = 0; i < num_pressed_keys; i++)
	{
		if (pressed_keys[i] == scancode)
		{
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  Add a key to the pressed keys list
  * @param  scancode: USB HID scancode
  * @retval None
  */
static void add_pressed_key(uint8_t scancode)
{
	if (num_pressed_keys < MAX_TRACKED_KEYS)
	{
		pressed_keys[num_pressed_keys++] = scancode;
	}
}

/**
  * @brief  Remove a key from the pressed keys list
  * @param  scancode: USB HID scancode
  * @retval None
  */
static void remove_pressed_key(uint8_t scancode)
{
	for (uint8_t i = 0; i < num_pressed_keys; i++)
	{
		if (pressed_keys[i] == scancode)
		{
			// Shift remaining keys
			for (uint8_t j = i; j < num_pressed_keys - 1; j++)
			{
				pressed_keys[j] = pressed_keys[j + 1];
			}
			num_pressed_keys--;
			break;
		}
	}
}

/**
  * @brief  Check if a scancode is in the keys array
  * @param  scancode: USB HID scancode to search for
  * @param  keys_array: Array of scancodes
  * @param  size: Size of the array
  * @retval 1 if found, 0 if not found
  */
static uint8_t is_key_in_array(uint8_t scancode, const uint8_t *keys_array, uint8_t size)
{
	for (uint8_t i = 0; i < size; i++)
	{
		if (keys_array[i] == scancode)
		{
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  Convert modifier bit position to USB HID scancode
  * @param  bit_position: Bit position (0-7) in modifier byte
  * @retval USB HID scancode (0xE0-0xE7)
  */
static uint8_t scancode_from_modifier_bit(uint8_t bit_position)
{
	// bit0=LCTRL(0xE0), bit1=LSHIFT(0xE1), ..., bit7=RGUI(0xE7)
	return 0xE0 + bit_position;
}

/**
  * @brief  Handle key press event
  * @param  scancode: USB HID scancode
  * @param  modifiers: Current modifier state
  * @param  timestamp: Event timestamp
  * @retval None
  */
static void handle_key_press(uint8_t scancode, uint8_t modifiers, uint32_t timestamp)
{
	// Check if this key is already pressed (safety check)
	if (is_key_pressed(scancode))
	{
		// Key is already in pressed list, ignore
		return;
	}

	// Add to pressed keys list
	add_pressed_key(scancode);

	// Print key press event - simple format
	DBG_N("[%lu ms] PRESS: 0x%02X (mod:0x%02X)\r\n", timestamp, scancode, modifiers);

	// Check if this is a lock key and toggle its state
	switch (scancode)
	{
		case SCANCODE_CAPS_LOCK:
			caps_lock_state = !caps_lock_state;
			send_led_status(caps_lock_state ? LED_CAPS_LOCK_ON : LED_CAPS_LOCK_OFF, timestamp);
			break;

		case SCANCODE_NUM_LOCK:
			num_lock_state = !num_lock_state;
			send_led_status(num_lock_state ? LED_NUM_LOCK_ON : LED_NUM_LOCK_OFF, timestamp);
			break;

		case SCANCODE_SCROLL_LOCK:
			scroll_lock_state = !scroll_lock_state;
			send_led_status(scroll_lock_state ? LED_SCROLL_LOCK_ON : LED_SCROLL_LOCK_OFF, timestamp);
			break;

		default:
			// Not a lock key, nothing to do
			break;
	}
}

/**
  * @brief  Handle key release event
  * @param  scancode: USB HID scancode
  * @param  modifiers: Current modifier state
  * @param  timestamp: Event timestamp
  * @retval None
  */
static void handle_key_release(uint8_t scancode, uint8_t modifiers, uint32_t timestamp)
{
	// Check if this key is in our pressed list
	if (!is_key_pressed(scancode))
	{
		// Key was not tracked as pressed, ignore
		return;
	}

	// Remove from pressed keys list
	remove_pressed_key(scancode);

	// Print key release event - simple format
	DBG_N("[%lu ms] RELEASE: 0x%02X (mod:0x%02X)\r\n", timestamp, scancode, modifiers);
}

/**
  * @brief  Send LED status message to USB task
  * @param  status: LED status to send
  * @param  timestamp: Event timestamp
  * @retval None
  */
static void send_led_status(led_status_t status, uint32_t timestamp)
{
	led_message_t msg;
	memset(&msg, 0, sizeof(msg));  // Initialize structure to zero
	msg.led_status = status;
	msg.timestamp = timestamp;

	osStatus_t result = osMessageQueuePut(ledQueueHandle, &msg, 0, 0);
	if (result != osOK)
	{
		// Queue full - silently ignore to avoid deadlock
	}
}

/**
  * @brief  Convert keyboard_message_t to keyboard_code_t format
  * @param  msg: Pointer to keyboard message (raw HID format)
  * @param  code: Pointer to keyboard code structure (amikb_process format)
  * @retval None
  */
static void convert_message_to_keyboard_code(keyboard_message_t *msg, keyboard_code_t *code)
{
	// Extract individual modifier flags from bitmap
	code->lctrl  = (msg->modifiers & 0x01) ? 1 : 0;
	code->lshift = (msg->modifiers & 0x02) ? 1 : 0;
	code->lalt   = (msg->modifiers & 0x04) ? 1 : 0;
	code->lgui   = (msg->modifiers & 0x08) ? 1 : 0;
	code->rctrl  = (msg->modifiers & 0x10) ? 1 : 0;
	code->rshift = (msg->modifiers & 0x20) ? 1 : 0;
	code->ralt   = (msg->modifiers & 0x40) ? 1 : 0;
	code->rgui   = (msg->modifiers & 0x80) ? 1 : 0;

	// The "pressed" fields are handled by amikb_process internally
	// Just initialize them to 0
	code->lctrlpressed  = 0;
	code->lshiftpressed = 0;
	code->laltpressed   = 0;
	code->lguipressed   = 0;
	code->rctrlpressed  = 0;
	code->rshiftpressed = 0;
	code->raltpressed   = 0;
	code->rguipressed   = 0;

	// Copy the 6 key slots
	memcpy(code->keys, msg->keys, 6);

	// Initialize keyspressed array to 0
	memset(code->keyspressed, 0, 6);
}

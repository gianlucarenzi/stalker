/**
  ******************************************************************************
  * @file           : amiga_task.c
  * @brief          : Amiga Task Implementation for FreeRTOS-based USB to Amiga Adapter
  ******************************************************************************
  * @details        This file implements the Amiga task responsible for:
  *                 - GPIO communication with Amiga computer
  *                 - Amiga keyboard protocol implementation and timing
  *                 - USB scancode to Amiga scancode conversion
  *                 - Reset condition detection and handling
  *                 - LED status management and communication with USB task
  *                 - Inter-task communication via FreeRTOS queues
  *
  * @author         Gianluca Renzi R.G. (RetroBitLab Tech Guy)
  * @version        v1.5-rtos
  * @date           2024
  * @copyright      (C) Copyright 2019/2024 by Gianluca Renzi
  * @license        SPDX-License-Identifier: LGPL-3.0-or-later
  *
  * @note           This task runs at medium priority (AMIGA_TASK_PRIORITY = 2) to handle
  *                 Amiga protocol timing requirements while allowing USB task precedence.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "amiga_task.h"
#include "debug.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @brief Interval for checking reset conditions (ms) */
#define RESET_CHECK_INTERVAL_MS     50

/** @brief Timeout for reset condition detection (ms) */
#define RESET_TIMEOUT_MS           500

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @brief Debug level for Amiga task logging */
static int debuglevel = DBG_INFO;

/** @brief FreeRTOS task handle for Amiga task */
TaskHandle_t amiga_task_handle = NULL;

/** @brief Current state of the Amiga task */
static task_state_t amiga_task_state = TASK_STATE_INIT;

/** @brief Flag indicating if Amiga interface is ready */
static int amiga_ready = 0;

/** @brief Timestamp when reset timer was started */
static TickType_t reset_timer_start = 0;

/** @brief Flag indicating if reset timer is active */
static int reset_timer_active = 0;

/* Private function prototypes -----------------------------------------------*/
static void amiga_task_process_keyboard_data(void);
static void amiga_task_check_reset_condition(void);
static void amiga_task_send_led_status(led_status_t status);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Amiga Task main function
  * @details Main task loop that handles Amiga keyboard protocol communication and
  *          reset condition monitoring. Runs at 50ms intervals to balance responsiveness
  *          with system efficiency. Task responsibilities:
  *          - Processing keyboard data from USB task
  *          - Converting USB scancodes to Amiga protocol
  *          - Monitoring for reset conditions from Amiga side
  *          - Managing LED status communication with USB task
  * @param  pvParameters: Task parameters (unused)
  * @retval None (task never returns)
  * @note   Task priority: AMIGA_TASK_PRIORITY (2 - Medium)
  *         Stack size: AMIGA_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)
  */
void amiga_task(void *pvParameters)
{
	TickType_t last_wake_time;
	const TickType_t task_frequency = pdMS_TO_TICKS(RESET_CHECK_INTERVAL_MS);
	
	DBG_I("Amiga Task started\r\n");
	
	amiga_task_state = TASK_STATE_RUNNING;
	
	/* Initialize the task */
	amiga_task_init();
	
	/* Initialize the xLastWakeTime variable with the current time */
	last_wake_time = xTaskGetTickCount();
	
	for (;;)
	{
		/* Process keyboard data from USB task */
		amiga_task_process_keyboard_data();
		
		/* Check for reset conditions */
		amiga_task_check_reset_condition();
		
		/* Wait for the next cycle */
		vTaskDelayUntil(&last_wake_time, task_frequency);
	}
}

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
void amiga_task_init(void)
{
	DBG_N("Amiga Task initialization\r\n");
	
	/* Initialize GPIO for Amiga */
	amikb_gpio_init();
	
	/* Start Amiga keyboard protocol */
	amikb_startup(1); // 1: use_OS vTaskDelay(), 0: don't use OS Timing (udelay, mdelay)
	
	/* Set Amiga as ready */
	amikb_ready(0); // Initially not ready until USB keyboard is connected
	
	DBG_N("Amiga Task initialization complete\r\n");
}

/**
  * @brief  Process keyboard data from USB task
  * @details Receives keyboard messages from the USB task via keyboard_queue and processes
  *          them through the Amiga keyboard protocol. Handles scancode conversion and
  *          LED status feedback. Updates Amiga ready state when first keyboard data is received.
  * @param  None
  * @retval None
  * @note   Processes all available messages in queue during each call
  *         Automatically sets Amiga interface to ready state on first keyboard data
  *         Forwards LED status changes to USB task via led_queue
  */
static void amiga_task_process_keyboard_data(void)
{
	keyboard_message_t kbd_msg;
	
	/* Check for keyboard messages from USB task */
	while (xQueueReceive(keyboard_queue, &kbd_msg, 0) == pdTRUE)
	{
		DBG_V("Received keyboard data from USB task\r\n");
		
		/* Process the keyboard data through Amiga protocol */
		led_status_t led_status = amikb_process(&kbd_msg.keycode);
		DBG_V("Led status: %d\r\n", led_status);
		amiga_task_send_led_status(led_status);
		
		/* Update Amiga ready state */
		if (!amiga_ready)
		{
			amiga_ready = 1;
			amikb_ready(1);
			DBG_I("Amiga keyboard interface is now ready\r\n");
		}
	}
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
	if (uxQueueMessagesWaiting(keyboard_queue) == 0)
	{
		/* Check if CLOCK line is being held low by external Amiga keyboard */
		if (amikb_reset_check())
		{
			/* Clock line is low - start or continue reset timer */
			if (!reset_timer_active)
			{
				reset_timer_start = xTaskGetTickCount();
				reset_timer_active = 1;
				DBG_V("Reset condition detected - starting timer\r\n");
			}
			else
			{
				/* Check if reset timeout has elapsed */
				TickType_t elapsed = xTaskGetTickCount() - reset_timer_start;
				if (elapsed >= pdMS_TO_TICKS(RESET_TIMEOUT_MS))
				{
					DBG_I("Reset timeout elapsed - performing Amiga reset\r\n");
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
				DBG_V("Reset condition cleared\r\n");
			}
		}
	}
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
void amiga_task_handle_reset(int use_OS)
{
	DBG_I("Performing Amiga reset sequence\r\n");
	
	/* Perform the reset */
	amikb_reset(use_OS);
	
	/* Restart the Amiga keyboard protocol */
	amikb_startup(use_OS);
	
	/* Send LED reset blink status to USB task */
	amiga_task_send_led_status(LED_RESET_BLINK);
	
	DBG_I("Amiga reset sequence complete\r\n");
}

/**
  * @brief  Send LED status to USB task
  * @details Sends LED status messages to the USB task via led_queue for keyboard LED control.
  *          Packages LED status with timestamp and handles queue send with timeout to avoid
  *          blocking the Amiga task if USB task is not responsive.
  * @param  status: LED status to send (LED_CAPS_LOCK_ON/OFF, LED_NUM_LOCK_ON/OFF, etc.)
  * @retval None
  * @note   Uses 10ms timeout for queue send to prevent task blocking
  *         Logs warning if message cannot be sent (queue full or USB task unresponsive)
  */
static void amiga_task_send_led_status(led_status_t status)
{
	led_message_t led_msg;
	led_msg.led_status = status;
	led_msg.timestamp = xTaskGetTickCount();

	if (xQueueSend(led_queue, &led_msg, pdMS_TO_TICKS(10)) != pdTRUE)
	{
		DBG_W("Failed to send LED status to USB task\r\n");
	}
	else
	{
		DBG_N("LED status %d sent to USB task\r\n", status);
		switch( status )
		{
			case LED_CAPS_LOCK_OFF:
				DBG_N("CAPS LOCK LED OFF\r\n");
				break;
				
			case LED_CAPS_LOCK_ON:
				DBG_N("CAPS LOCK LED ON\r\n");
				break;
				
			case LED_NUM_LOCK_OFF:
				DBG_N("NUM LOCK LED OFF\r\n");
				break;
				
			case LED_NUM_LOCK_ON:
				DBG_N("NUM LOCK LED ON\r\n");
				break;
				
			case LED_SCROLL_LOCK_OFF:
				DBG_N("SCROLL LOCK LED OFF\r\n");
				break;
				
			case LED_SCROLL_LOCK_ON:
				DBG_N("SCROLL LOCK LED ON\r\n");
				break;
				
			case LED_RESET_BLINK:
				DBG_N("Reset occurred from Amiga Side - reinitializing keyboard LEDs\r\n");
				break;
			
			case NO_LED:
			default:
				DBG_N("NO ACTION FOR LEDs\r\n");
				break;
		}
	}
}

/**
  * @brief  Get Amiga task state
  * @details Returns the current state of the Amiga task for monitoring and debugging purposes.
  * @param  None
  * @retval task_state_t: Current task state (TASK_STATE_INIT, TASK_STATE_RUNNING, etc.)
  * @note   Used for task health monitoring and system diagnostics
  */
task_state_t amiga_task_get_state(void)
{
	return amiga_task_state;
}

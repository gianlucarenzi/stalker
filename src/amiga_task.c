/**
 * Amiga Task Implementation
 * Handles GPIO communication with Amiga and scancode processing
 * 
 * Written by Gianluca Renzi R.G.
 * (C) Copyright 2019/2024 by Gianluca Renzi (RetroBitLab Tech Guy)
 * 
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "amiga_task.h"
#include "debug.h"
#include "main.h"

/* Debug level */
static int debuglevel = DBG_INFO;

/* Task handle */
TaskHandle_t amiga_task_handle = NULL;

/* Task state */
static task_state_t amiga_task_state = TASK_STATE_INIT;

/* Amiga state variables */
static int amiga_ready = 0;
static TickType_t reset_timer_start = 0;
static int reset_timer_active = 0;

/* Reset check timing */
#define RESET_CHECK_INTERVAL_MS     50
#define RESET_TIMEOUT_MS           500

/* Function prototypes */
static void amiga_task_process_keyboard_data(void);
static void amiga_task_check_reset_condition(void);
static void amiga_task_send_led_status(led_status_t status);

/**
 * @brief Amiga Task main function
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
 * @brief Initialize Amiga Task
 */
void amiga_task_init(void)
{
    DBG_N("Amiga Task initialization\r\n");
    
    /* Initialize GPIO for Amiga */
    amikb_gpio_init();
    
    /* Start Amiga keyboard protocol */
    amikb_startup();
    
    /* Set Amiga as ready */
    amikb_ready(0); // Initially not ready until USB keyboard is connected
    
    DBG_N("Amiga Task initialization complete\r\n");
}

/**
 * @brief Process keyboard data from USB task
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
        
        /* Send LED status back to USB task if needed */
        if (led_status != NO_LED)
        {
            amiga_task_send_led_status(led_status);
        }
        
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
 * @brief Check for reset conditions from Amiga side
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
                    amiga_task_handle_reset();
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
 * @brief Handle Amiga reset
 */
void amiga_task_handle_reset(void)
{
    DBG_I("Performing Amiga reset sequence\r\n");
    
    /* Perform the reset */
    amikb_reset();
    
    /* Restart the Amiga keyboard protocol */
    amikb_startup();
    
    /* Send LED reset blink status to USB task */
    amiga_task_send_led_status(LED_RESET_BLINK);
    
    DBG_I("Amiga reset sequence complete\r\n");
}

/**
 * @brief Send LED status to USB task
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
        DBG_V("LED status %d sent to USB task\r\n", status);
    }
}

/**
 * @brief Get Amiga task state
 */
task_state_t amiga_task_get_state(void)
{
    return amiga_task_state;
}
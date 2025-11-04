/**
 * @file extended_bridge.c
 * @brief Bridge task for converting extended HID usages to Amiga key sequences.
 *
 * This task reads from the extended_input_queue, maps HID usages to
 * CTRL+ALT+Fn key combinations, and injects them into the Amiga keyboard
 * stream via the keyboard_inject_queue. It manages modifier states to
 * prevent key sticking and redundant presses.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "debug.h"
#include "hid_report.h"
#include "task_communication.h"
#include "usbh_hid_keybd.h"

#define EXTENDED_BRIDGE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)
#define EXTENDED_BRIDGE_TASK_PRIORITY   3

static TaskHandle_t extended_bridge_task_handle = NULL;

static int debuglevel = DBG_INFO;

// Maps HID consumer usage to a function key
static uint8_t usage_to_function_key(uint16_t usage) {
    switch (usage) {
        // Consumer Page (0x0C)
        case 0xE2: return KEY_F1;  // Mute
        case 0xEA: return KEY_F2;  // Volume Down
        case 0xE9: return KEY_F3;  // Volume Up
        case 0xCD: return KEY_F4;  // Play/Pause
        case 0xB5: return KEY_F5;  // Next Track
        case 0xB6: return KEY_F6;  // Previous Track
        case 0xB7: return KEY_F7;  // Stop

        // Keyboard/Keypad Page (0x07)
        case 0x46: return KEY_F8; // PrintScreen
        case 0x65: return KEY_F9; // Application/Menu

        default: return 0;
    }
}

/**
 * @brief The main task for the extended HID bridge.
 *
 * This task waits for extended HID input events, maps them to key sequences,
 * and injects them into the Amiga keyboard handler.
 *
 * @param pvParameters Unused.
 */
static void extended_to_amiga_task(void *pvParameters)
{
    hid_input_event_t event;
    uint8_t function_key;
    static bool active_usages[256] = {false};
    static int ctrl_ref_count = 0;
    static int alt_ref_count = 0;
    static keyboard_code_t injected_keycode = {0};

    DBG_I("Extended to Amiga bridge task started\r\n");

    for (;; ) {
        if (xQueueReceive(extended_input_queue, &event, portMAX_DELAY) == pdPASS) {
            function_key = usage_to_function_key(event.usage);
            if (function_key == 0) {
                continue; // No mapping for this usage
            }

            keyboard_message_t msg;
            msg.timestamp = xTaskGetTickCount();

            if (event.pressed && !active_usages[event.usage % 256]) { // Press
                active_usages[event.usage % 256] = true;

                if (ctrl_ref_count == 0) {
                    injected_keycode.lctrl = 1;
                    msg.keycode = injected_keycode;
                    xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);
                }
                ctrl_ref_count++;

                if (alt_ref_count == 0) {
                    injected_keycode.lalt = 1;
                    msg.keycode = injected_keycode;
                    xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);
                }
                alt_ref_count++;

                // Find a free slot for the function key
                int i;
                for (i = 0; i < KEY_PRESSED_MAX; i++) {
                    if (injected_keycode.keys[i] == 0) {
                        injected_keycode.keys[i] = function_key;
                        break;
                    }
                }
                msg.keycode = injected_keycode;
                xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);

            } else if (!event.pressed && active_usages[event.usage % 256]) { // Release
                active_usages[event.usage % 256] = false;

                // Find and remove the function key
                int i;
                for (i = 0; i < KEY_PRESSED_MAX; i++) {
                    if (injected_keycode.keys[i] == function_key) {
                        injected_keycode.keys[i] = 0;
                        break;
                    }
                }
                msg.keycode = injected_keycode;
                xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);

                alt_ref_count--;
                if (alt_ref_count == 0) {
                    injected_keycode.lalt = 0;
                    msg.keycode = injected_keycode;
                    xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);
                }

                ctrl_ref_count--;
                if (ctrl_ref_count == 0) {
                    injected_keycode.lctrl = 0;
                    msg.keycode = injected_keycode;
                    xQueueSend(keyboard_inject_queue, &msg, (TickType_t)10);
                }
            }
        }
    }
}

/**
 * @brief Initializes and creates the extended bridge task.
 */
void extended_bridge_init_create_task(void)
{
    if (keyboard_inject_queue == NULL) {
        DBG_E("keyboard_inject_queue is NULL, cannot create extended bridge task\r\n");
        return;
    }

    if (xTaskCreate(extended_to_amiga_task, "ExtBridge", EXTENDED_BRIDGE_TASK_STACK_SIZE, NULL, EXTENDED_BRIDGE_TASK_PRIORITY, &extended_bridge_task_handle) != pdPASS) {
        DBG_E("Failed to create extended bridge task\r\n");
    }
}
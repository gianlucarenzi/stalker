#ifndef __TASK_COMMUNICATION_H__
#define __TASK_COMMUNICATION_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "amiga.h"

/* Queue sizes */
#define KEYBOARD_QUEUE_SIZE     10
#define LED_QUEUE_SIZE          5

/* Queue handles - declared as extern, defined in main.c */
extern QueueHandle_t keyboard_queue;
extern QueueHandle_t led_queue;

/* Task priorities */
#define USB_TASK_PRIORITY       3
#define AMIGA_TASK_PRIORITY     2

/* Task stack sizes */
#define USB_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 4)
#define AMIGA_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 2)

/* Communication structures */
typedef struct {
    keyboard_code_t keycode;
    uint32_t timestamp;
} keyboard_message_t;

typedef struct {
    led_status_t led_status;
    uint32_t timestamp;
} led_message_t;

/* Task states */
typedef enum {
    TASK_STATE_INIT = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_ERROR,
    TASK_STATE_SUSPENDED
} task_state_t;

#endif /* __TASK_COMMUNICATION_H__ */
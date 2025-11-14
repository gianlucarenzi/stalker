#ifndef __AMIGA_INCLUDED__
#define __AMIGA_INCLUDED__

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usb_task.h"  // Include for keyboard_code_t, led_status_t, keyboard_led_t

extern void amikb_startup(int use_OS);                                // Must be called within the Amiga Task
extern led_status_t amikb_process(keyboard_code_t *data, int use_OS); // Must be called within the Main Task
extern void amikb_notify(const char *notify);                         // Must be called within the Main Task
extern void amikb_gpio_init(void);                                    // Must be called within the Amiga Task
extern void amikb_ready(int isready);                                 // ?? Maybe both?
extern bool amikb_reset_check(void);                                  // Must be called within the Amiga Task
extern void amikb_reset(int use_OS);                                  // Must be called within the Amiga Task
extern void ll_amikb_send(uint8_t code, int press);                   // Must be called within the Amiga Task
extern led_status_t amikb_send(uint8_t code, int press, int use_OS);

// New declarations for non-static functions
extern uint8_t scancode_to_amiga(uint8_t lkey);
extern uint8_t ascii_to_scancode(uint8_t ascii);

typedef enum {
	AMIGA_DO_STARTUP = 0,
	AMIGA_DO_RESET,
	AMIGA_PROCESS_KEY,
	AMIGA_LAST, // Should be the last
} amiga_state_t;

typedef enum {
	TYPE_EMPTY = 0,
	TYPE_KEYBOARD_CODE,
} data_type_t;

typedef struct {
	uint8_t keycode;
	int press;
} key_status_t;

typedef struct {
	amiga_state_t state;
	data_type_t   type;
	void*         data;
} message_t;

extern QueueHandle_t queue;

typedef enum {
	AMIGA_MODE = 0,
	PC_MODE,
} reset_keypress_mode_t;

/* Amiga Reset timeout in milliseconds */
#define RESET_TIMEOUT_MS 500

#endif

#ifndef __AMIGA_INCLUDED__
#define __AMIGA_INCLUDED__

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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

extern void amikb_startup(void);                          // Must be called within the Amiga Task
extern led_status_t amikb_process(keyboard_code_t *data); // Must be called within the Main Task
extern void amikb_notify(const char *notify);             // Must be called within the Main Task
extern void amikb_gpio_init(void);                        // Must be called within the Amiga Task
extern void amikb_ready(int isready);                     // ?? Maybe both?
extern bool amikb_reset_check(void);                      // Must be called within the Amiga Task
extern void amikb_reset(void);                            // Must be called within the Amiga Task
extern void ll_amikb_send(uint8_t code, int press);       // Must be called within the Amiga Task

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

#endif

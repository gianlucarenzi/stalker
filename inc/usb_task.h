#ifndef __USB_TASK_H__
#define __USB_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usb_host.h"
#include "task_communication.h"

/* Task handle */
extern TaskHandle_t usb_task_handle;

/* USB Task function */
void usb_task(void *pvParameters);

/* USB Task initialization */
void usb_task_init(void);

/* USB LED management functions */
void usb_keyboard_led_set(USBH_HandleTypeDef *usbhost, keyboard_led_t led);
void usb_keyboard_led_init_sequence(USBH_HandleTypeDef *usbhost);
void led_toggle(void);

/* USB Task state management */
task_state_t usb_task_get_state(void);

#endif /* __USB_TASK_H__ */
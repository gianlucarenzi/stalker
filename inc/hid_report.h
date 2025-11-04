#ifndef __HID_REPORT_H__
#define __HID_REPORT_H__

#include <stdint.h>
#include "usbh_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Event types supported by extended HID parsing */
typedef enum {
    HID_EVT_KEYBOARD = 0,
    HID_EVT_CONSUMER,
    HID_EVT_SYSTEM
} hid_event_type_t;

/* Unified input event from HID reports */
typedef struct {
    hid_event_type_t type;   /* Keyboard, Consumer, System */
    uint16_t usage;          /* HID Usage ID (raw) */
    uint8_t pressed;         /* 1 = press, 0 = release */
    uint32_t timestamp;      /* system tick or provided time */
} hid_input_event_t;

/* Initialize per-interface report handling (placeholder) */
int hid_report_init_for_interface(USBH_HandleTypeDef *phost, uint8_t interface_number);

/* Parse and cache report descriptor for an interface (placeholder) */
int hid_report_parse_descriptor(USBH_HandleTypeDef *phost, uint8_t interface_number);

/* Decode a single input report buffer and emit events via callback (placeholder) */
int hid_report_decode(USBH_HandleTypeDef *phost,
                      uint8_t interface_number,
                      const uint8_t *report,
                      uint16_t report_len,
                      void (*event_cb)(const hid_input_event_t *evt));

#ifdef __cplusplus
}
#endif

#endif /* __HID_REPORT_H__ */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "debug.h"
#include "task_communication.h"
#include "hid_report.h"

/* Public queue handle declared in main.c */
extern QueueHandle_t extended_input_queue;

/* Serial log queue and task */
#ifndef SERIAL_LOG_QUEUE_SIZE
#define SERIAL_LOG_QUEUE_SIZE  32
#endif
#ifndef SERIAL_LOG_MAXLEN
#define SERIAL_LOG_MAXLEN      128
#endif

static QueueHandle_t serial_log_queue = NULL;
static TaskHandle_t serial_logger_task_handle = NULL;
static TaskHandle_t extended_logger_task_handle = NULL;

/* UART write primitive (uses printf backend). If you prefer HAL_UART_Transmit, replace here. */
static void serial_write(const char* s, uint16_t len)
{
    /* Ensure string is null-terminated when using printf */
    for (uint16_t i = 0; i < len; ++i) {
        putchar((int)s[i]);
    }
}

int log_enqueue(const char* msg, uint16_t len)
{
    if (!serial_log_queue) {
        /* Fallback early: write directly to UART/printf */
        if (len > 0 && msg) serial_write(msg, len);
        return 0;
    }
    if (len > SERIAL_LOG_MAXLEN) len = SERIAL_LOG_MAXLEN;
    char buf[SERIAL_LOG_MAXLEN];
    memcpy(buf, msg, len);
    return (xQueueSend(serial_log_queue, buf, 0) == pdTRUE) ? 0 : -1;
}

int log_enqueuef(const char* fmt, ...)
{
    char buf[SERIAL_LOG_MAXLEN];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (n >= (int)sizeof(buf)) n = sizeof(buf) - 1;
    buf[n] = '\0';
    if (!serial_log_queue) {
        /* Fallback early */
        serial_write(buf, (uint16_t)strnlen(buf, sizeof(buf)));
        return 0;
    }
    return (xQueueSend(serial_log_queue, buf, 0) == pdTRUE) ? 0 : -1;
}

static void serial_logger_task(void* arg)
{
    (void)arg;
    char msg[SERIAL_LOG_MAXLEN];
    for(;;) {
        if (xQueueReceive(serial_log_queue, msg, portMAX_DELAY) == pdTRUE) {
            size_t len = strnlen(msg, sizeof(msg));
            serial_write(msg, (uint16_t)len);
        }
    }
}

static void extended_logger_task(void* arg)
{
    (void)arg;
    hid_input_event_t evt;
    for(;;) {
        if (xQueueReceive(extended_input_queue, &evt, portMAX_DELAY) == pdTRUE) {
            const char* type_str = "UNKNOWN";
            const char* color = ANSI_CYAN;
            switch (evt.type) {
                case HID_EVT_KEYBOARD: type_str = "KBD"; color = ANSI_LIGHT_GREEN; break;
                case HID_EVT_CONSUMER: type_str = "CONS"; color = ANSI_LIGHT_CYAN; break;
                case HID_EVT_SYSTEM:   type_str = "SYS"; color = ANSI_LIGHT_MAGENTA; break;
                default: break;
            }
            log_enqueuef("%s[HID-EXT %s] usage=%u pressed=%u ts=%lu%s\r\n",
                         color, type_str, (unsigned)evt.usage, (unsigned)evt.pressed, (unsigned long)evt.timestamp, ANSI_RESET);
        }
    }
}

void logger_init_create_tasks(void)
{
    if (!serial_log_queue) {
        serial_log_queue = xQueueCreate(SERIAL_LOG_QUEUE_SIZE, SERIAL_LOG_MAXLEN);
    }
    if (serial_log_queue && !serial_logger_task_handle) {
        xTaskCreate(serial_logger_task, "serial_logger", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &serial_logger_task_handle);
    }
    if (!extended_logger_task_handle) {
        xTaskCreate(extended_logger_task, "ext_logger", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &extended_logger_task_handle);
    }
}

#ifndef __AMIGA_TASK_H__
#define __AMIGA_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "amiga.h"
#include "task_communication.h"

/* Task handle */
extern TaskHandle_t amiga_task_handle;

/* Amiga Task function */
void amiga_task(void *pvParameters);

/* Amiga Task initialization */
void amiga_task_init(void);

/* Amiga Task state management */
task_state_t amiga_task_get_state(void);

/* Amiga reset management */
void amiga_task_handle_reset(void);

#endif /* __AMIGA_TASK_H__ */
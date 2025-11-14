/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : amiga_task.h
  * @brief          : Header for amiga_task.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AMIGA_TASK_H
#define __AMIGA_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
	AMIGA_TASK_MSG_RESET_START,
} AmigaTaskMsg_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Amiga Task entry point
  * @param  argument: Not used
  * @retval None
  */
void amigaTask(void *argument);

/* External queue handles that this task uses --------------------------------*/
extern osMessageQueueId_t amigaTaskQueueHandle;     /**< Internal message queue for amiga_task */
extern osMessageQueueId_t keyboardQueueHandle;  /**< USB -> Amiga keyboard data */
extern osMessageQueueId_t ledQueueHandle;       /**< Amiga -> USB LED status */

#ifdef __cplusplus
}
#endif

#endif /* __AMIGA_TASK_H */

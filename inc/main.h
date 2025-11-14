/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#define KBD_CLOCK_Pin       GPIO_PIN_2
#define KBD_CLOCK_GPIO_Port GPIOC
#define KBD_DATA_Pin        GPIO_PIN_3
#define KBD_DATA_GPIO_Port  GPIOC
#define KBD_RESET_Pin       GPIO_PIN_1
#define KBD_RESET_GPIO_Port GPIOC
#define TP1_Pin             GPIO_PIN_0
#define TP1_GPIO_Port       GPIOA
#define TP2_Pin             GPIO_PIN_1
#define TP2_GPIO_Port       GPIOA
#define LED_PIN_Pin         TP1_Pin
#define LED_PIN_GPIO_Port   TP1_GPIO_Port
#define DEBUG_TX_Pin        GPIO_PIN_2
#define DEBUG_TX_GPIO_Port  GPIOA
#define DEBUG_RX_Pin        GPIO_PIN_3
#define DEBUG_RX_GPIO_Port  GPIOA
#define USB_DM_Pin          GPIO_PIN_11
#define USB_DM_GPIO_Port    GPIOA
#define USB_DP_Pin          GPIO_PIN_12
#define USB_DP_GPIO_Port    GPIOA
#define USBH_Port           GPIOA

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

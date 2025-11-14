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

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdio.h>

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
#define BOOT_MODE_Pin GPIO_PIN_1
#define BOOT_MODE_GPIO_Port GPIOC
#define LED_PIN_Pin GPIO_PIN_0
#define LED_PIN_GPIO_Port GPIOA
#define UART_TX_Pin GPIO_PIN_2
#define UART_TX_GPIO_Port GPIOA
#define UART_RX_Pin GPIO_PIN_3
#define UART_RX_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
// ANSI Color Codes
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Debug Levels
#define DEBUG_LEVEL_NONE    0 // Only errors
#define DEBUG_LEVEL_INFO    1 // Errors + Info
#define DEBUG_LEVEL_DEBUG   2 // Errors + Debug
#define DEBUG_LEVEL_ALL     3 // Errors + Info + Debug

// Configure current debug level here
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_ALL

// Software Version
#define SOFTWARE_VERSION "1.0"

// Custom Print Macros
#define PRINT_NORMAL(fmt, ...)  printf(ANSI_COLOR_RESET fmt ANSI_COLOR_RESET, ##__VA_ARGS__)
#define PRINT_ERROR(fmt, ...)   printf(ANSI_COLOR_RED "ERROR: " fmt ANSI_COLOR_RESET, ##__VA_ARGS__)

#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define PRINT_INFO(fmt, ...)    printf(ANSI_COLOR_GREEN "INFO: " fmt ANSI_COLOR_RESET, ##__VA_ARGS__)
#else
#define PRINT_INFO(fmt, ...)
#endif

#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
#define PRINT_DEBUG(fmt, ...)   printf(ANSI_COLOR_YELLOW "DEBUG: " fmt ANSI_COLOR_RESET, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(fmt, ...)
#endif

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

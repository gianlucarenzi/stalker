/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for USB to Amiga Keyboard Adapter
  ******************************************************************************
  * @details        This file contains the main application entry point and
  *                 system initialization for the FreeRTOS-based USB to Amiga
  *                 keyboard adapter. The application creates two main tasks:
  *                 - USB Task: Handles USB HID communication and LED management
  *                 - Amiga Task: Handles Amiga keyboard protocol and GPIO
  *
  * @author         RetrobitLab
  * @version        v2.0-rtos
  * @date           2025
  *
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "usb_host.h"
#include "syscall.h"
#include "debug.h"
#include "stm32f4xx_it.h"
#include "amiga.h"
#include "eeprom.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Task includes */
#include "task_communication.h"
#include "usb_task.h"
#include "amiga_task.h"

EepromMode current_mode = AMIGA_MODE; // Default mode

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @brief Global queue handle for keyboard data communication (USB -> Amiga) */
QueueHandle_t keyboard_queue = NULL;

/** @brief Global queue handle for LED status communication (Amiga -> USB) */
QueueHandle_t led_queue = NULL;

/** @brief Debug level for application logging */
static int debuglevel = DBG_INFO;

/** @brief Firmware build version string with timestamp */
static const char *fwBuild = "v2.0.1-rc-RTOS BUILD: " __TIME__ "-" __DATE__;

/** @brief UART handle for debug communication */
static UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(int baud);
static void create_tasks_and_queues(void);
static void banner(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Display startup banner with firmware information
  * @details Prints colorized banner with firmware version and build information
  *          to the debug UART. Includes optional Amiberry easter egg message.
  * @param  None
  * @retval None
  */
static void banner(void)
{
	printf("\r\n\r\n" ANSI_BLUE "RETROBITLAB AMIGA USB KEYBOARD ADAPTER" ANSI_RESET "\r\n");
#ifdef __AMIBERRY_EASTER_EGG__
	printf(ANSI_LIGHT_MAGENTA "Amiberry Dimitris Panokostas VERSION" ANSI_RESET "\r\n");
#endif
	printf(ANSI_YELLOW);
	printf("FWVER: %s", fwBuild);
	printf(ANSI_RESET "\r\n");
	printf("\r\n\n");
}

/**
  * @brief  Create and initialize FreeRTOS tasks and communication queues
  * @details Creates the main application tasks and inter-task communication queues:
  *          - keyboard_queue: For USB to Amiga keyboard data (size: KEYBOARD_QUEUE_SIZE)
  *          - led_queue: For Amiga to USB LED status (size: LED_QUEUE_SIZE)
  *          - USB Task: High priority task for USB HID management
  *          - Amiga Task: Medium priority task for Amiga protocol handling
  * @param  None
  * @retval None
  * @note   Calls _Error_Handler() if any task or queue creation fails
  */
static void create_tasks_and_queues(void)
{
	/* Create communication queues */
	keyboard_queue = xQueueCreate(KEYBOARD_QUEUE_SIZE, sizeof(keyboard_message_t));
	if (keyboard_queue == NULL)
	{
		DBG_E("Failed to create keyboard queue\r\n");
		_Error_Handler(__FILE__, __LINE__);
	}

	led_queue = xQueueCreate(LED_QUEUE_SIZE, sizeof(led_message_t));
	if (led_queue == NULL)
	{
		DBG_E("Failed to create LED queue\r\n");
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Create USB task */
	if (xTaskCreate(usb_task, "USB_Task", USB_TASK_STACK_SIZE, NULL, USB_TASK_PRIORITY, &usb_task_handle) != pdPASS)
	{
		DBG_E("Failed to create USB task\r\n");
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Create Amiga task */
	if (xTaskCreate(amiga_task, "Amiga_Task", AMIGA_TASK_STACK_SIZE, NULL, AMIGA_TASK_PRIORITY, &amiga_task_handle) != pdPASS)
	{
		DBG_E("Failed to create Amiga task\r\n");
		_Error_Handler(__FILE__, __LINE__);
	}

	DBG_I("Tasks and queues created successfully\r\n");
}

/**
  * @brief  The application entry point
  * @details Main function that initializes the system and starts the FreeRTOS scheduler.
  *          Performs the following initialization sequence:
  *          1. HAL library initialization
  *          2. System clock configuration
  *          3. GPIO initialization
  *          4. Debug UART initialization
  *          5. FreeRTOS tasks and queues creation
  *          6. FreeRTOS scheduler start
  * @param  None
  * @retval None (function never returns under normal operation)
  * @note   If the scheduler returns, it indicates a critical error and the system
  *         enters an infinite error loop
  */
int main(void)
{
	// Read EEPROM configuration
	uint32_t eeprom_value;

	_write_ready(SYSCALL_NOTREADY, &huart2);

	/* MCU Configuration----------------------------------------------------------*/
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	
	/* Disable WWDG if it was enabled by bootloader */
	if (__HAL_RCC_WWDG_IS_CLK_ENABLED())
	{
		/* Disable WWDG clock to prevent unwanted interrupts */
		__HAL_RCC_WWDG_CLK_DISABLE();
	}
	
	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();

	/* Initialize DEBUG UART */
	MX_USART2_UART_Init(115200);
	_write_ready(SYSCALL_READY, &huart2);

	banner();

	DBG_I("Starting FreeRTOS-based USB to Amiga Keyboard Adapter\r\n");

	if (eeprom_read(EEPROM_MODE_CONFIG, &eeprom_value) == HAL_OK)
	{
		if (eeprom_value == 0xFFFFFFFF)
		{
			DBG_I("EEPROM is uninitialized. Saving default mode (AMIGA_MODE)...\r\n");
			if (eeprom_write(EEPROM_MODE_CONFIG, AMIGA_MODE) == HAL_OK)
			{
				DBG_I("Default mode saved successfully.\r\n");
				current_mode = AMIGA_MODE;
			}
			else
			{
				DBG_E("Failed to save default mode to EEPROM.\r\n");
			}
		}
		else
		{
			current_mode = (EepromMode)eeprom_value;
			if (current_mode == AMIGA_MODE)
			{
				DBG_I("EEPROM Mode: AMIGA_MODE\r\n");
			}
			else if (current_mode == PC_MODE)
			{
				DBG_I("EEPROM Mode: PC_MODE\r\n");
			}
			else
			{
				DBG_W("EEPROM Mode: Unknown value (0x%lx), defaulting to AMIGA_MODE\r\n", eeprom_value);
				current_mode = AMIGA_MODE;
			}
		}
	}
	else
	{
		DBG_E("Failed to read EEPROM_MODE_CONFIG, defaulting to AMIGA_MODE\r\n");
	}

	/* Create tasks and queues */
	create_tasks_and_queues();

	/* Start the FreeRTOS scheduler */
	DBG_I("Starting FreeRTOS scheduler\r\n");
	vTaskStartScheduler();

	/* We should never get here as control is now taken by the scheduler */
	DBG_E("FreeRTOS scheduler returned - this should never happen!\r\n");
	for (;;)
	{
		/* If we get here, there was insufficient memory to create the idle task */
		_Error_Handler(__FILE__, __LINE__);
	}
}

/**
  * @brief  System Clock Configuration
  * @details Configures the system clock to run at maximum frequency using HSE.
  *          Clock configuration:
  *          - HSE: 8 MHz external crystal
  *          - PLL: HSE/4 * 168 / 4 = 84 MHz (SYSCLK)
  *          - AHB: 84 MHz (HCLK)
  *          - APB1: 42 MHz (PCLK1)
  *          - APB2: 84 MHz (PCLK2)
  *          - SysTick: 1 kHz (1 ms tick)
  * @param  None
  * @retval None
  * @note   Calls _Error_Handler() if clock configuration fails
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/**Configure the main internal regulator output voltage
	*/
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/**Initializes the CPU, AHB and APB busses clocks
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
	_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	*/
	RCC_ClkInitStruct.ClockType =
		RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
		RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure the Systick interrupt time
	*/
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	/**Configure the Systick
	*/
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}

/**
  * @brief  USART2 Initialization Function
  * @details Configures USART2 for debug communication with specified baud rate.
  *          Configuration:
  *          - Data bits: 8
  *          - Stop bits: 1
  *          - Parity: None
  *          - Flow control: None
  *          - Mode: TX/RX
  * @param  baud: Baud rate for UART communication (typically 115200)
  * @retval None
  * @note   Calls _Error_Handler() if UART initialization fails
  */
static void MX_USART2_UART_Init(int baud)
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = baud;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/**
  * @brief  GPIO Initialization Function
  * @details Configure pins as:
  *          - TP1_Pin (Test Point 1): Output, Push-Pull, No Pull, Low Speed
  *          - TP2_Pin (Test Point 2): Output, Push-Pull, No Pull, Low Speed
  *          Both pins are initially set to HIGH state.
  * @param  None
  * @retval None
  * @note   Enables GPIO clocks for ports A, B, and H
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();


	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(TP1_GPIO_Port, TP1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(TP2_GPIO_Port, TP2_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin : TP1_Pin & TP2_Pin */
	GPIO_InitStruct.Pin = TP1_Pin | TP2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(TP1_GPIO_Port, &GPIO_InitStruct);

}

/**
  * @brief  This function is executed in case of error occurrence
  * @details Error handler that enters an infinite loop when a critical error occurs.
  *          This function is called by various HAL functions and application code
  *          when an unrecoverable error is detected.
  * @param  file: The file name as string where the error occurred
  * @param  line: The line number in file where the error occurred
  * @retval None (function never returns)
  * @note   In debug builds, this function can be used to set breakpoints
  *         for error analysis
  */
void _Error_Handler(char *file, int line)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred
  * @details This function is called when an assertion fails in HAL library functions.
  *          It provides debugging information about parameter validation errors.
  * @param  file: pointer to the source file name where assertion failed
  * @param  line: assert_param error line source number
  * @retval None (function never returns)
  * @note   This function is only compiled when USE_FULL_ASSERT is defined
  */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
	DBG_E("Error! Wrong parameters value: file %s on line %d\r\n", file, line);
	while (1)
	{
	}
}
#endif /* USE_FULL_ASSERT */

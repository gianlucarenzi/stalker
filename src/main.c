/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_host.h"
#include <stdio.h>
#include <stdbool.h>
#include "assert.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "log_task.h"
#include "delay_us.h"
#include "debug.h"
#include "usb_task.h"
#include "amiga.h"
#include "amiga_task.h"
#include "led_manager_task.h"
#include "eeprom_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for USBKeyBoardTask */
osThreadId_t USBKeyBoardTaskHandle;
const osThreadAttr_t USBKeyBoardTask_attributes = {
	.name = "USBKeyBoardTask",
	.stack_size = 1024,  // Increased from 512 to 1024 for new functionality
	.priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AmigaTask */
osThreadId_t AmigaTaskHandle;
const osThreadAttr_t AmigaTask_attributes = {
	.name = "AmigaTask",
	.stack_size = 1024,  // Increased from 512 to 1024
	.priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LedManagerTask */
osThreadId_t LedManagerTaskHandle;
const osThreadAttr_t LedManagerTask_attributes = {
	.name = "LedManagerTask",
	.stack_size = 512,
	.priority = (osPriority_t) osPriorityLow,
};

/** @brief Global queue handle for keyboard data communication (USB -> Amiga) */
osMessageQueueId_t keyboardQueueHandle;
const osMessageQueueAttr_t keyboardQueueHandle_attributes = {
	.name = "keyboardQueue"
};

/** @brief Current keyboard mode (AMIGA_MODE or PC_MODE) */
volatile reset_keypress_mode_t current_mode = AMIGA_MODE;

/** @brief Global queue handle for LED status communication (Amiga -> USB) */
osMessageQueueId_t ledQueueHandle;
const osMessageQueueAttr_t ledQueueHandle_attributes = {
		.name = "ledQueue"
};

/** @brief Global queue handle for LED manager commands */
osMessageQueueId_t ledManagerQueueHandle;
const osMessageQueueAttr_t ledManagerQueueHandle_attributes = {
		.name = "ledManagerQueue"
};

/** @brief Global queue handle for internal amiga task messages */
osMessageQueueId_t amigaTaskQueueHandle;
const osMessageQueueAttr_t amigaTaskQueueHandle_attributes = {
		.name = "amigaTaskQueue"
};

/* USER CODE BEGIN PV */

static int debuglevel = DBG_INFO;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE END 0 */

/** @brief Firmware build version string with timestamp */
static const char *fwBuild = "v3.1NG-RTOS BUILD: " __TIME__ "-" __DATE__;
/**
  * @brief  Display startup banner with firmware information
  * @details Prints colorized banner with firmware version and build information
  *          to the debug UART. Includes optional Amiberry easter egg message.
  * @param  None
  * @retval None
  */
static void banner(void)
{
	printf("\r\n\r\n" ANSI_BLUE "RETROBITLAB AMIGA USB KEYBOARD ADAPTER V2" ANSI_RESET "\r\n");
#ifdef __AMIBERRY_EASTER_EGG__
	printf(ANSI_LIGHT_MAGENTA "Amiberry Dimitris Panokostas VERSION" ANSI_RESET "\r\n");
#endif
	printf(ANSI_YELLOW);
	printf("FWVER: %s", fwBuild);
	printf(ANSI_RESET "\r\n");
	printf("\r\n\n");
}

static void create_system_queues(void)
{
	/* creation of usbLedQueue */
	ledQueueHandle = osMessageQueueNew (16, sizeof(led_message_t), &ledQueueHandle_attributes);
	if (ledQueueHandle == NULL)
	{
		DBG_E("Error Creating ledQueueHandle\r\n");
		assert(ledQueueHandle == NULL);
	}
	else
	{
		DBG_V("ledQueueHandle created %p.\r\n", ledQueueHandle);
	}

	ledManagerQueueHandle = osMessageQueueNew (32, sizeof(led_manager_message_t), &ledManagerQueueHandle_attributes);
	if (ledManagerQueueHandle == NULL)
	{
		DBG_E("Error Creating ledManagerQueueHandle\r\n");
		assert(ledManagerQueueHandle == NULL);
	}
	else
	{
		DBG_V("ledManagerQueueHandle created %p.\r\n", ledManagerQueueHandle);
	}

	keyboardQueueHandle = osMessageQueueNew (16, sizeof(keyboard_message_t), &keyboardQueueHandle_attributes);
	if (keyboardQueueHandle == NULL)
	{
		DBG_E("Error Creating keyboardQueueHandle\r\n");
		assert(keyboardQueueHandle == NULL);
	}
	else
	{
		DBG_V("keyboardQueueHandle created %p.\r\n", keyboardQueueHandle);
	}

	amigaTaskQueueHandle = osMessageQueueNew (8, sizeof(AmigaTaskMsg_t), &amigaTaskQueueHandle_attributes);
	if (amigaTaskQueueHandle == NULL)
	{
		DBG_E("Error Creating amigaTaskQueueHandle\r\n");
		assert(amigaTaskQueueHandle == NULL);
	}
	else
	{
		DBG_V("amigaTaskQueueHandle created %p.\r\n", amigaTaskQueueHandle);
	}
	DBG_I("System Queues Created.\r\n");
}

static void create_system_tasks(void)
{
	/* creation of USBKeyBoardTask */
	USBKeyBoardTaskHandle = osThreadNew(usbKeyboardTask, NULL, &USBKeyBoardTask_attributes);
	if (USBKeyBoardTaskHandle == NULL)
	{
		DBG_E("Error Creating Task USBKeyBoardTaskHandle\r\n");
		assert(USBKeyBoardTaskHandle == NULL);
	}
	else
	{
		DBG_V("USBKeyBoardTaskHandle created %p.\r\n", USBKeyBoardTaskHandle);
	}

	/* creation of AmigaTask */
	AmigaTaskHandle = osThreadNew(amigaTask, NULL, &AmigaTask_attributes);
	if (AmigaTaskHandle == NULL)
	{
		DBG_E("Error Creating Task AmigaTaskHandle\r\n");
		assert(AmigaTaskHandle == NULL);
	}
	else
	{
		DBG_V("AmigaTaskHandle created %p.\r\n", AmigaTaskHandle);
	}

	/* creation of LedManagerTask */
	LedManagerTaskHandle = osThreadNew(LedManagerTask, NULL, &LedManagerTask_attributes);
	if (LedManagerTaskHandle == NULL)
	{
		DBG_E("Error Creating Task LedManagerTaskHandle\r\n");
		assert(LedManagerTaskHandle == NULL);
	}
	else
	{
		DBG_V("LedManagerTaskHandle created %p.\r\n", LedManagerTaskHandle);
	}
	DBG_I("System Tasks Created\r\n");
}

#define PANIC_BLINK_RATE 500000L
static void panic(void)
{
	int j;
	/* USER CODE END WHILE */
	HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_RESET);
	for(j=0; j < PANIC_BLINK_RATE; j++)
	{
		;;
	}
	HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_SET);
	for(j=0; j < PANIC_BLINK_RATE; j++)
	{
		;;
	}
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	/* MCU Configuration--------------------------------------------------------*/
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();

	DelayUs_Init();
	LogTask_Init(&huart2);

	banner();

	DBG_I("Starting FreeRTOS-based USB to Amiga Keyboard Adapter Next Gen\r\n");

	/* Init scheduler */
	osKernelInitialize();

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* Create the queue(s) */
	create_system_queues();

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* Init and create the EEPROM management task */
	eeprom_task_init();

	create_system_tasks();

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */
	DBG_E("#### Unexpected RTOS Kernel/System Failure ####\r\n");
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE BEGIN 3 */
		panic();
		/* USER CODE END 3 */
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
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
	Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
	Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
	Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level for LED */
  HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level for Keyboard pins */
  HAL_GPIO_WritePin(KBD_CLOCK_GPIO_Port, KBD_CLOCK_Pin, GPIO_PIN_SET);    // KBD_CLOCK high
  HAL_GPIO_WritePin(KBD_DATA_GPIO_Port, KBD_DATA_Pin, GPIO_PIN_SET);      // KBD_DATA high
  HAL_GPIO_WritePin(KBD_RESET_GPIO_Port, KBD_RESET_Pin, GPIO_PIN_RESET);  // KBD_RESET low

  /*Configure GPIO pin : PA0 (LED_PIN) */
  GPIO_InitStruct.Pin = LED_PIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PIN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC1 (KBD_RESET), PC2 (KBD_CLOCK), PC3 (KBD_DATA) */
  GPIO_InitStruct.Pin = KBD_RESET_Pin|KBD_CLOCK_Pin|KBD_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC13 PC14 PC15 PC0
						   PC4 PC5 PC6 PC7 PC8
						   PC9 PC10 PC11 PC12
						   (PC1, PC2, PC3 excluded - used for KBD) */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
						  |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
						  |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
						  |GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA4 PA5
						   PA6 PA7 PA8 PA9
						   PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5
						  |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9
						  |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
						   PB12 PB13 PB14 PB15
						   PB3 PB4 PB5 PB6
						   PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
						  |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
						  |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
						  |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PD2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
	HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  printf("!!! Error_Handler called !!!\r\n");
  __disable_irq();
  panic();
  /* USER CODE END Error_Handler_Debug */
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  printf("!!! STACK OVERFLOW in task: %s !!!\r\n", pcTaskName);
  while(1);
}

/* FreeRTOS malloc failed hook */
void vApplicationMallocFailedHook(void)
{
  printf("!!! MALLOC FAILED - Heap exhausted !!!\r\n");
  while(1);
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  printf("Wrong parameters value: file %s on line %d\r\n", file, line);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

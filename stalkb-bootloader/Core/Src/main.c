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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (*pFunction)(void);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STM32_DFU_ROM_CODE 0x1FFF0000
#define DFU_ENTRY_OFFSET 4 // Offset for Reset Handler (second word)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define APPLICATION_NAME "STALKER"
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// External linker symbols for application and RAM addresses
extern uint32_t __appflash_start;
extern uint32_t __appflash_end;
extern uint32_t __ram_start;
extern uint32_t __ram_end;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static uint8_t check_boot_mode_pin(void);
static void jump_to_dfu_bootloader(void);
static int application_is_valid(void);
static void application_run(void);
static void print_startup_banner(void);
static void show_dfu_mode(int use_dfu);
static void amiga_reset(void);
/* USER CODE END PFP */

/* Public function prototypes -------------------------------------------------*/
void SystemClock_Config(void);

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
__attribute__((weak)) void custom_setup_early(void) 
{
	/* Do something useful here for your custom hardware */
}

__attribute__((weak)) void custom_setup_late(void) 
{
	/* Do something useful here for your custom hardware */
	amiga_reset();
}

/* USER CODE END 0 */
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
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin : LED_PIN_Pin */
	GPIO_InitStruct.Pin = LED_PIN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_PIN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BOOT_MODE_Pin */
	GPIO_InitStruct.Pin = BOOT_MODE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(BOOT_MODE_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static uint8_t check_boot_mode_pin(void)
{
	uint32_t samples_sum = 0;
	const uint32_t num_samples = 20;
	const uint32_t delay_ms = 1000 / num_samples; // 50ms delay for 20 samples over 1 second

	//PRINT_INFO("Checking BOOT_MODE_Pin for 1 second...\r\n");

	for (uint32_t i = 0; i < num_samples; i++)
	{
		samples_sum += HAL_GPIO_ReadPin(BOOT_MODE_GPIO_Port, BOOT_MODE_Pin);
		HAL_Delay(delay_ms);
	}

	// If average is closer to 0 (less than half are high), assume DFU mode
	if (samples_sum < (num_samples / 2))
	{
		//PRINT_INFO("BOOT_MODE_Pin detected as LOW (DFU mode).\r\n");
		return 1; // DFU mode
	}
	else
	{
		//PRINT_INFO("BOOT_MODE_Pin detected as HIGH (Normal mode).\r\n");
		return 0; // Normal mode
	}
}

static void jump_to_dfu_bootloader(void)
{
	PRINT_INFO("Jumping to DFU Bootloader...\r\n");

	// Get the DFU entry point (reset handler address)
	// The entry point is typically stored at the second word (offset 4) of the vector table
	pFunction dfu_entry_point = (pFunction)(*(volatile uint32_t *)(STM32_DFU_ROM_CODE + DFU_ENTRY_OFFSET));

	// Set the main stack pointer to the start of the DFU ROM
	__set_MSP(*(volatile uint32_t *)STM32_DFU_ROM_CODE);

	// Jump to the DFU entry point
	dfu_entry_point();

	// Should not return from here
	while (1)
	{
		// Error: DFU jump failed or returned
		PRINT_ERROR("DFU jump failed or returned!\r\n");
	}
}

static int application_is_valid(void)
{
	uint32_t app_stack_pointer = *(volatile uint32_t *)&__appflash_start;
	pFunction app_reset_handler = (pFunction)(*(volatile uint32_t *)((uint32_t)&__appflash_start + 4));

	PRINT_INFO("Checking for application firmware at 0x%lx...\r\n", (long unsigned int)&__appflash_start);

	// Validate application's stack pointer and reset handler address
	if ((app_stack_pointer < (uint32_t)&__ram_start) || (app_stack_pointer > (uint32_t)&__ram_end))
	{
		PRINT_ERROR("Application stack pointer (0x%lx) is outside RAM range (0x%lx - 0x%lx).\r\n",
			   (long unsigned int)app_stack_pointer, (long unsigned int)&__ram_start, (long unsigned int)&__ram_end);
		return 0;
	}

	if ( ((uint32_t)app_reset_handler < (uint32_t)&__appflash_start) || ((uint32_t)app_reset_handler > (uint32_t)&__appflash_end) )
	{
		PRINT_ERROR("Application reset handler (0x%lx) is outside application start address (0x%lx) or available flash area.\r\n",
			   (long unsigned int)app_reset_handler, (long unsigned int)&__appflash_start);
		return 0;
	}

	/* Application is valid and can be launched */
	return 1;
}

static void application_run(void)
{
	uint32_t app_stack_pointer = *(volatile uint32_t *)&__appflash_start;
	pFunction app_reset_handler = (pFunction)(*(volatile uint32_t *)((uint32_t)&__appflash_start + 4));

	PRINT_INFO("Valid application firmware found. Jumping to application...\r\n");

	// Turn off the LED before jumping to application
	HAL_GPIO_WritePin(LED_PIN_GPIO_Port, LED_PIN_Pin, GPIO_PIN_SET);

	//__disable_irq();

	// Set the Vector Table Offset Register to the application's vector table
	SCB->VTOR = (uint32_t)&__appflash_start;

	// Set the Main Stack Pointer to the application's stack pointer
	__set_MSP(app_stack_pointer);

	// Jump to the application's reset handler
	app_reset_handler();

	// Should not return from here
	while (1)
	{
		// Error: Application jump failed or returned
		PRINT_ERROR("Application jump failed or returned!\r\n");
	}
}

static void print_startup_banner(void)
{
	PRINT_INFO("\x1b[36m**********************************\x1b[0m\r\n"); // Cyan
	PRINT_INFO("\x1b[36m*  \x1b[33mSTM32 " APPLICATION_NAME " USB BOOTLOADER\x1b[36m  *\x1b[0m\r\n"); // Cyan borders, Yellow text
	PRINT_INFO("\x1b[36m**********************************\x1b[0m\r\n"); // Cyan
	PRINT_INFO("\x1b[0m\r\n"); // Reset color and add a newline for spacing
	PRINT_INFO("----------------------------------------\r\n");
	PRINT_INFO("STM32 " APPLICATION_NAME " BOOTLOADER\r\n");
	PRINT_INFO("Version: %s\r\n", SOFTWARE_VERSION);
	PRINT_INFO("Build Timestamp: %s %s\r\n", __DATE__, __TIME__);
}

static void show_dfu_mode(int use_dfu)
{
	PRINT_INFO("User request DFU MODE: %s\r\n", use_dfu == 1 ? ANSI_COLOR_RED"YES"ANSI_COLOR_RESET : ANSI_COLOR_CYAN"NO"ANSI_COLOR_RESET);
	PRINT_INFO("----------------------------------------\r\n");
}

/**
 * @brief Keep Amiga in reset state during update
 * @retval None
 */
static void amiga_reset(void)
{
	/* Configure AMIGA RESET as output after checking the bootloader
	 * user request
	 */
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*Configure GPIO pin Output Level BOOT_MODE_Pin / AMIGA RESET as low */
	HAL_GPIO_WritePin(BOOT_MODE_GPIO_Port, BOOT_MODE_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin as output: PC1 - AMIGA RESET J8 */
	GPIO_InitStruct.Pin = BOOT_MODE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP; /* Due to a short circuit tied to gnd an internal pullup is needed */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOOT_MODE_GPIO_Port, &GPIO_InitStruct);
	PRINT_INFO("Amiga now it's in RESET state\r\n");
}

/* USER CODE END 4 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	int use_dfu;
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();

	/* USER CODE BEGIN 2 */
	custom_setup_early();

	print_startup_banner(); // Display banner at startup

	use_dfu = check_boot_mode_pin();

	show_dfu_mode(use_dfu); // Display banner at startup

	custom_setup_late();

	/* USER CODE END 2 */
	if (application_is_valid())
	{
		if (!use_dfu)
		{
			application_run();
		}
	}

	jump_to_dfu_bootloader();

	PRINT_ERROR("*** NEVER REACHED ***\r\n");
	__disable_irq();
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include <string.h> // for memset
#include "main.h"
#include "syscall.h"
#include "debug.h"

/* From Linker file */
extern uint32_t _estack;

/* Local variables */
static int debuglevel = DBG_INFO;
static const char *fwBuild = "v0.2 BUILD: " __TIME__ "-" __DATE__;

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(int baudrate);

/*
 * Reads NUM_SAMPLES and manage the average
 */
#define NUM_SAMPLES 10
#define WAIT_MS (1000L / NUM_SAMPLES)

#define STM32_DFU_ROM_CODE 0x1FFF0000
#define BOOTLOADER_SIZE    0x4000 /* 16K Bootloader size */

#define BOOT_1_PIN      GPIO_PIN_1 //STALKER V2 BOARD: PC1
#define BOOT_1_PORT     GPIOC
#define BOOT_1_ENABLED  GPIO_PIN_RESET
#define LED_1_PIN       GPIO_PIN_0
#define LED_1_PORT      GPIOA

static int ask_for_bootloader(void);
static void banner(void);
static void amiga_reset(void);

/**
 * @brief Keep Amiga in reset state during update
 * @retval None
 */
static void amiga_reset(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*Configure GPIO pin Output Level BOOT_1_PIN as low */
	HAL_GPIO_WritePin(BOOT_1_PORT, BOOT_1_PIN, GPIO_PIN_RESET);

	/*Configure GPIO pin as output: PC1 - AMIGA RESET J8 */
	GPIO_InitStruct.Pin = BOOT_1_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP; /* Due to a short circuit tied to gnd an internal pullup is needed */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOOT_1_PORT, &GPIO_InitStruct);
}

static void banner(void)
{
	printf("\r\n\r\n" ANSI_BLUE "RETROBITLAB STM32 USB DFU BOOTLOADER" ANSI_RESET "\r\n");
	printf(ANSI_YELLOW);
	printf("FWVER: %s", fwBuild);
	printf(ANSI_RESET "\r\n");
	printf("\r\n\n");
}

static int ask_for_bootloader(void)
{
	int boot_pin [ NUM_SAMPLES ];
	int count = 0;
	int no_boot = 0;
	int maybe_boot = 0;

	memset(boot_pin, 0, sizeof(int) * NUM_SAMPLES);

	while (count < NUM_SAMPLES)
	{
		boot_pin[ count ] = HAL_GPIO_ReadPin(BOOT_1_PORT, BOOT_1_PIN);
		HAL_Delay(WAIT_MS);
		count++;
	}

	for (count = 0; count < NUM_SAMPLES; count++)
	{
		if (boot_pin[ count ] != BOOT_1_ENABLED)
			no_boot++;
		else
			maybe_boot++;
	}

	/* We need at least the half or more pressions of the pin tied to ground
	 * to be sure we are in bootmode...
	 */
	if (maybe_boot >= no_boot)
		return 1;
	return 0;
}

/**
 * @brief It is a sanity check for valid vector pointers to SRAM and INTERNAL FLASH
 * @retval Returns 0 if NO VALID APP FOUND otherwise 1
 */
static inline int check_valid_application(uint32_t jumpAddress, uint32_t stackAddress)
{
	int ram_is_valid = 0;
	int flash_is_valid = 0;

	DBG_N("ApplicationAddressSpace = 0x%08lX -- FLASH_BASE: 0x%08lX\r\n", jumpAddress, FLASH_BASE);
	DBG_N("ApplicationRAMSpace     = 0x%08lX -- SRAM_BASE: 0x%08lX\r\n", stackAddress, SRAM_BASE);

	ram_is_valid = stackAddress >= SRAM_BASE && stackAddress <= (SRAM_BASE + 0x10000);
	flash_is_valid = jumpAddress >= (FLASH_BASE + BOOTLOADER_SIZE) && jumpAddress < FLASH_END;

	DBG_N("RIV: %d -- FIV: %d\r\n", ram_is_valid, flash_is_valid);

	DBG_V("The Application is %s\r\n", (ram_is_valid && flash_is_valid) ? "VALID" : "INVALID");
	return (ram_is_valid && flash_is_valid);
}

static void mdump(uint32_t offset, uint32_t size)
{
	for (uint32_t reg = 0; reg < (size * 4); reg = reg + 4)
	{
		uint32_t regval = *(__IO uint32_t *) (offset + reg);
		printf("[%02ld] 0x%08lX\r\n", reg, regval);
	}
}

/**
  * @brief  The USB Bootloader.
  * @retval never reached
  * 
  * Tie the Amiga Reset Pin to ground for at least 1 second during
  * powerup to enter into the bootloader mode, otherwise it will jump
  * to user application.
  * 
  */
int main(void)
{
	int bootmode;
	typedef void (*pFunction)(void);
	pFunction Jump_To_Application;
	uint32_t JumpAddress;
	uint32_t StackAddress;

	_write_ready(SYSCALL_NOTREADY, &huart2);

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init(115200);
	_write_ready(SYSCALL_READY, &huart2);

	banner();

	bootmode = ask_for_bootloader();
	/* Now we need to reconfigure pin as output, as well as the Amiga needs
	 * to be in reset mode during all upgrade (if connected)
	 */
	amiga_reset();

	if ( !bootmode ) {
		DBG_I("NFU\r\n");
		/*
		 * It's a tricky calculation: basically every firmware starts with
		 * the Vector Table (Jump table) and it is defined in the startup
		 * code (.s) as:
		 *
		 *
				g_pfnVectors:
				  .word  _estack
				  .word  Reset_Handler
				  .word  NMI_Handler
				  .word  HardFault_Handler
				  .word  MemManage_Handler
				  .word  BusFault_Handler
				  .word  UsageFault_Handler
				  .word  0
				  .word  0
				  .word  0
				  .word  0
				  .word  SVC_Handler
				  .word  DebugMon_Handler
				  .word  0
				  .word  PendSV_Handler
				  .word  SysTick_Handler
		 *
		 * So, the g_pfnVectors is located in the first (FLASH_BASE) address
		 * and it contains at offset + 0 the _estack STACK POINTER (usually
		 * located in RAM) and the following address (+1 word or +4 bytes)
		 * is the Reset_Handler (i.e. the start of the application firmware
		 * code).
		 *
		 * What I can see here, is the following issue:
		 *
		 * If there is no code at that address (first programmed bootloader)
		 * the machine crash if we don't force the bootloader mode.
		 *
		 * It should be great if a sanity check is done BEFORE entering in
		 * the jump code. Example of sanity check:
		 *
		 * uint32_t stackPtr;
		 * uint32_t resetPtr;
		 * stackPtr = *(uint32_t *)(FLASH_BASE + BOOTLOADER_SIZE);
		 * resetPtr = *(uint32_t *)(FLASH_BASE + BOOTLOADER_SIZE + 4);
		 *
		 * The stackPtr must be reside within the RAM SPACE Address, meanwhile
		 * the resetPtr must be equal or higher than the application flash
		 * address (FLASH_BASE + BOOTLOADER_SIZE) and less than the last
		 * valid flash address (FLASH_BASE + BOOTLOADER_SIZE + FLASH_SIZE)
		 *
		 */
		JumpAddress = *(__IO uint32_t*) (FLASH_BASE + BOOTLOADER_SIZE + 4);
		StackAddress = *(__IO uint32_t*) (FLASH_BASE + BOOTLOADER_SIZE + 0);

		if (check_valid_application(JumpAddress, StackAddress)) {

			Jump_To_Application = (pFunction) JumpAddress;

			/* Set the vector table */
			SCB->VTOR = FLASH_BASE + BOOTLOADER_SIZE;

			uint32_t msp_value = *(__IO uint32_t *) (FLASH_BASE + BOOTLOADER_SIZE);

			/* Set the STACK POINTER to the Application space */
			__set_MSP(msp_value);
			Jump_To_Application();
			/* Never reached */
			while (1) ;
		} else {
			DBG_E("Not valid application found. Firmware Upgrade FORCED.\r\n");
			HAL_Delay(100);
		}
	}

	DBG_I("Starting STM32 DFU BOOTLOADER Mode...\r\n");
	HAL_Delay(200);

	/* Enter in DFU ROM Code. */
	JumpAddress = *(__IO uint32_t*) (STM32_DFU_ROM_CODE + 4);
	Jump_To_Application = (pFunction) JumpAddress;
	__set_MSP(*(__IO uint32_t *) (STM32_DFU_ROM_CODE));
	Jump_To_Application();
	/* Never reached */
	while (1)
	{
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
static void MX_USART2_UART_Init(int baudrate)
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = baudrate;
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
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level LED D4 ON */
	HAL_GPIO_WritePin(LED_1_PORT, LED_1_PIN, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC1 - AMIGA RESET J8 */
	GPIO_InitStruct.Pin = BOOT_1_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOOT_1_PORT, &GPIO_InitStruct);

	/*Configure GPIO pin : PA0 */
	GPIO_InitStruct.Pin = LED_1_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_1_PORT, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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
	DBG_E("Error! Wrong parameters value: file %s on line %d\r\n", file, line);
	while (1)
	{
	}
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

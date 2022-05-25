/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
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
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "usb_host.h"
#include "syscall.h"
#include "debug.h"
#include "stm32f4xx_it.h"
#include "amiga.h"

/* External functions */
extern void MX_USB_HOST_Process(void);

/* Local functions prototypes */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(int baud);

/* Local variables */
static int debuglevel = DBG_NOISY;
static const char *fwBuild = "v1.1rc";
static UART_HandleTypeDef huart2;

static void banner(void)
{
	printf("\r\n\r\n" ANSI_BLUE "RETROBITLAB AMIGA USB KEYBOARD ADAPTER" ANSI_RESET "\r\n");
	printf(ANSI_BLUE "-=* STM32F401 BASED BOARD HANDLER  *=-" ANSI_RESET "\r\n");
	printf(ANSI_YELLOW);
	printf("FWVER: %s", fwBuild);
	printf(ANSI_RESET "\r\n");
	printf("\r\n\n");
}

static void led_light(int state)
{
	int tpval = GPIO_PIN_RESET;

	if (!!state)
	{
		tpval = GPIO_PIN_SET;
	}
	else
	{
		tpval = GPIO_PIN_RESET;
	}
	HAL_GPIO_WritePin(TP1_GPIO_Port, TP1_Pin, tpval);
}

static void led_toggle(void)
{
	static int tpval = 0;
	if (tpval == 0)
	{
		tpval = 1;
	}
	else
	{
		tpval = 0;
	}
	led_light(tpval);
}

#define USB_REPORT_RETRY    (6)

static void usb_keyboard_led(USBH_HandleTypeDef *usbhost, keyboard_led_t ld)
{
	keyboard_led_t led = ld;
	USBH_StatusTypeDef status;
	int retrygood = USB_REPORT_RETRY;
	DBG_N("Called: %p -- Led: 0x%02x\r\n", usbhost, led);
	if (usbhost != NULL)
	{
		for (;;)
		{
			status = USBH_HID_SetReport(usbhost, 0x02, 0x00, &led, 1);
			DBG_N("[%d] USB Status: %d\r\n", retrygood, status);
			if (status == USBH_OK)
				retrygood--;
			if (retrygood <= 0)
				break;
		}
	}
	DBG_N("Exit\r\n");
}

static void usb_keyboard_led_init(USBH_HandleTypeDef * usbhost)
{
	keyboard_led_t led = CAPS_LOCK_LED | NUM_LOCK_LED | SCROLL_LOCK_LED;
	int n;
	DBG_N("Called: led: 0x%02x\r\n", led);
	if (usbhost != NULL)
	{
		for (n = 0; n < 2; n++)
		{
			DBG_V("LEDS ON\r\n");
			usb_keyboard_led(usbhost, led);
			mdelay(250);
			DBG_V("LEDS OFF\r\n");
			usb_keyboard_led(usbhost, 0);
			mdelay(125);
		}
		/* Default leds off */
		DBG_V("LEDS OFF\r\n");
		usb_keyboard_led(usbhost, 0);
	}
	DBG_N("Exit\r\n");
}

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
	ApplicationTypeDef aState = APPLICATION_DISCONNECT;
	int timerOneShot = 1;
	int resetTimer = 0;
	led_status_t stat;
	static keyboard_led_t keyboard_led = 0;
	int do_led = 0;
	USBH_HandleTypeDef * usbhost = NULL;
	int keyboard_ready = 0;
	int count = 0;

	_write_ready(SYSCALL_NOTREADY, &huart2);

	/* MCU Configuration----------------------------------------------------------*/
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();
	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();

	/* Initialize DEBUG UART */
	MX_USART2_UART_Init(115200);
	_write_ready(SYSCALL_READY, &huart2);
	/* Initialize GPIO for Amiga and assert the nRESET Line */
	amikb_gpio_init();

	banner();

	/* Initialize USB HOST OTG FS */
	MX_USB_HOST_Init();

	/* Infinite loop */
	timer_start();
	/* Now intialize Amiga Pinouts and sync the keyboard */
	amikb_startup();
	amikb_ready(0);

	for (;;)
	{
		MX_USB_HOST_Process();
		aState = USBH_ApplicationState();
		// Se risulta connessa la tastiera USB
		if (aState == APPLICATION_READY)
		{
			DBG_N("APPLICATION READY\r\n");
			usbhost = USBH_GetHost();
			if (usbhost != NULL)
			{
				if (USBH_HID_GetDeviceType(usbhost) == HID_KEYBOARD)
				{
					if (!timerOneShot)
					{
						timerOneShot = !timerOneShot;
						DBG_V("#### KEYBOARD CONNECTED ####\r\n");
						timer_start();
					}

					if ( !keyboard_ready )
					{
						DBG_V("### BOARD LED ON ### WAIT 500msec FOR LEDS\r\n");
						led_light(0);
						if (timer_elapsed(500))
						{
							DBG_V("### KEYBOARD LED TOGGLE ###\r\n");
							usb_keyboard_led_init(usbhost);
							keyboard_ready = 1;
							keyboard_led = 0;
						}
					}

					// Get data from keyboard!
					if (USBH_Keybd(usbhost) == 0)
					{
						// Send all received keycode to Amiga
						stat = amikb_process(USBH_GetScanCode());
						// ...and manage the keyboard led
						switch (stat)
						{
							case LED_CAPS_LOCK_OFF:
								DBG_V("CAPS LOCK LED OFF\r\n");
								keyboard_led &= ~CAPS_LOCK_LED;
								do_led = 1;
								break;
							case LED_CAPS_LOCK_ON:
								DBG_V("CAPS LOCK LED ON\r\n");
								keyboard_led |= CAPS_LOCK_LED;
								do_led = 1;
								break;
							case LED_NUM_LOCK_OFF:
								DBG_V("NUM LOCK LED OFF\r\n");
								keyboard_led &= ~NUM_LOCK_LED;
								do_led = 1;
								break;
							case LED_NUM_LOCK_ON:
								DBG_V("NUM LOCK LED ON\r\n");
								keyboard_led |= NUM_LOCK_LED;
								do_led = 1;
								break;
							case LED_SCROLL_LOCK_OFF:
								DBG_V("SCROLL LOCK LED OFF\r\n");
								keyboard_led &= ~SCROLL_LOCK_LED;
								do_led = 1;
								break;
							case LED_SCROLL_LOCK_ON:
								DBG_V("SCROLL LOCK LED ON\r\n");
								keyboard_led |= SCROLL_LOCK_LED;
								do_led = 1;
								break;
							case LED_RESET_BLINK:
								DBG_V("RESET OCCURS FROM AMIGA SIDE\r\n");
								usb_keyboard_led_init(usbhost);
								do_led = 0;
								break;
							default:
							case NO_LED:
								DBG_V("NO ACTION FOR USB REPORT\r\n");
								do_led = 0;
								break;
						}
						// ...and let the led management to be done!
						if (do_led)
						{
							DBG_N("USB ASK FOR REPORT: LED: 0x%02x\r\n", keyboard_led);
							usb_keyboard_led(usbhost, keyboard_led);
						}
					}
					else
					{
						// In IDLE mode, check if there are some
						// RESET request on the CLOCK line.
						// Any EXTERNAL Amiga keyboard will assert low
						// the clock line for more than 500msec to
						// obtain the SYSTEM RESET REQUEST, so do we.
						if (amikb_reset_check())
						{
							// Is it the first time in IDLE state?
							if (!resetTimer)
							{
								// Yes! Startup the timer
								resetTimer = 1;
								timer_start();
							}
							// In reset state
							if (timer_elapsed(500))
							{
								DBG_N("Now it's time for a RESET!\r\n");
								resetTimer = 0;
								amikb_reset();
								amikb_startup();
							}
						}
						else
						{
							// If not in CLOCK LOW MODE, reset the
							// timer for next time...
							resetTimer = 0;
						}
					}
				}
				else
				{
					keyboard_ready = 0;
					// Quick blink on device-not-supported
					DBG_N("#### HID Device NOT SUPPORTED ####\r\n");
					if (!timerOneShot)
					{
						timerOneShot = !timerOneShot;
						timer_start();
					}
					if (timer_elapsed(100))
					{
						DBG_N("UNKNOWN USB DEVICE count: %d\r\n", count);
						led_toggle();
						amikb_notify("NOT USB Keyboard. Please Connect - Amiga Is Back!\n");
						timer_start();
						if (count++ > 10)
						{
							DBG_I("Waiting REAL USB Keyboard - Amiga Is Back!\r\n");
							count = 0;
						}
					}
				}
			}
			else
			{
				// Pretty quick blink on no-hid-device-plugged
				DBG_N("NO HID DEVICE FOUND.\r\n");
				keyboard_ready = 0;
				if (!timerOneShot)
				{
					timerOneShot = !timerOneShot;
					timer_start();
				}
				if (timer_elapsed(250))
				{
					DBG_N("NO HID DEVICE FOUND\r\n");
					led_toggle();
					amikb_notify("NO HID Device. Please Connect - Amiga Is Back!\n");
					timer_start();
					if (count++ > 10)
					{
						DBG_I("Waiting USB HID Keyboard - Amiga Is Back!\r\n");
						count = 0;
					}
				}
			}
		}
		else
		{
			keyboard_ready = 0;
			DBG_N("APPLICATION %d\r\n", aState);
			// On first run, we start a timer and every 1/2 second we
			// toggle LED Pin
			if (timerOneShot)
			{
				DBG_N("UNCONNECTED USB HID KEYBOARD. PLEASE CONNECT\r\n");
				timer_start();
				timerOneShot = 0;
			}
			// slow blink on no device connected
			if (timer_elapsed(500))
			{
				DBG_N("WAIT INSERT USB KEYBOARD count: %d\r\n", count);
				led_toggle();
				timer_start();
				if (count++ > 10)
				{
					amikb_notify("Waiting USB Keyboard - Amiga Is Back!\n");
					DBG_I("Waiting USB Keyboard - Amiga Is Back!\r\n");
					count = 0;
				}
			}
		}
		amikb_ready(keyboard_ready);
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
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

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USART2 init function */
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

/** Configure pins as
		* Analog
		* Input
		* Output
		* EVENT_OUT
		* EXTI
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
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
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
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
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

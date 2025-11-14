/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file            : usb_host.c
  * @version         : v1.0_Cube
  * @brief           : This file implements the USB Host
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

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"

/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostFS;
static volatile ApplicationTypeDef Appli_state = APPLICATION_IDLE;
static osMutexId_t Appli_state_mutex = NULL;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB host library, add supported class and start the library
  * @retval None
  */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */

  /* Create mutex for Appli_state protection */
  const osMutexAttr_t Appli_state_mutex_attr = {
    .name = "Appli_state_mutex"
  };
  Appli_state_mutex = osMutexNew(&Appli_state_mutex_attr);
  if (Appli_state_mutex == NULL) {
    Error_Handler();
  }

  /* USER CODE END USB_HOST_Init_PreTreatment */

  /* Init host Library, add supported class and start the library. */
  if (USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostFS, USBH_HID_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostFS) != USBH_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */

  /* USER CODE END USB_HOST_Init_PostTreatment */
}

/*
 * user callback definition
 */
static void USBH_UserProcess  (USBH_HandleTypeDef *phost, uint8_t id)
{
  /* USER CODE BEGIN CALL_BACK_1 */
  switch(id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
  break;

  case HOST_USER_DISCONNECTION:
  osMutexAcquire(Appli_state_mutex, osWaitForever);
  Appli_state = APPLICATION_DISCONNECT;
  osMutexRelease(Appli_state_mutex);
  // After a brief delay, return to IDLE state
  osDelay(100);  // Give time for disconnect message to be displayed
  osMutexAcquire(Appli_state_mutex, osWaitForever);
  Appli_state = APPLICATION_IDLE;
  osMutexRelease(Appli_state_mutex);
  break;

  case HOST_USER_CLASS_ACTIVE:
  osMutexAcquire(Appli_state_mutex, osWaitForever);
  Appli_state = APPLICATION_READY;
  osMutexRelease(Appli_state_mutex);
  break;

  case HOST_USER_CONNECTION:
  osMutexAcquire(Appli_state_mutex, osWaitForever);
  Appli_state = APPLICATION_START;
  osMutexRelease(Appli_state_mutex);
  break;

  default:
  break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/**
  * @brief  Get current application state (thread-safe)
  * @param  phost: USB Host handle (unused, for API consistency)
  * @retval ApplicationTypeDef Current application state
  * @note   This function uses a mutex to provide thread-safe access to Appli_state
  */
ApplicationTypeDef USBH_GetApplicationState(USBH_HandleTypeDef *phost)
{
  ApplicationTypeDef state;

  /* Check for NULL pointer */
  if (phost == NULL) {
    return APPLICATION_DISCONNECT;
  }

  /* Acquire mutex before reading */
  osMutexAcquire(Appli_state_mutex, osWaitForever);

  /* Read the state */
  state = Appli_state;

  /* Release mutex */
  osMutexRelease(Appli_state_mutex);

  return state;
}

/**
  * @}
  */

/**
  * @}
  */


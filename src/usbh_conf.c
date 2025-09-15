/**
  ******************************************************************************
  * @file           : usbh_conf.c
  * @version        : v1.0_Cube
  * @brief          : This file implements the board support package for the USB host library
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2025 STMicroelectronics International N.V.
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

#include "usbh_core.h"
#include "stm32f4xx_it.h"
#include "debug.h"

HCD_HandleTypeDef hhcd_USB_OTG_FS;
extern void _Error_Handler(char * file, int line);
static int debuglevel = DBG_INFO;

/*******************************************************************************
                       LL Driver Callbacks (HCD -> USB Host Library)
*******************************************************************************/
/* MSP Init */

void HAL_HCD_MspInit(HCD_HandleTypeDef* hcdHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	DBG_N("Called with handle: %p\r\n", hcdHandle);

	if (hcdHandle->Instance==USB_OTG_FS)
	{
		/**USB_OTG_FS GPIO Configuration
		PA11     ------> USB_OTG_FS_DM
		PA12     ------> USB_OTG_FS_DP
		*/
		DBG_V("Configuring Pins USB_DM and USB_DP\r\n");
		GPIO_InitStruct.Pin = USB_DM_Pin | USB_DP_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* Peripheral clock enable */
		__HAL_RCC_USB_OTG_FS_CLK_ENABLE();

		/* Peripheral interrupt init */
		HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
	} else {
		DBG_E("hcdHandle->Instance NOT USB_OTG_FS\r\n");
	}
	DBG_N("Exit\r\n");
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef* hcdHandle)
{
	DBG_N("Called with handle: %p\r\n", hcdHandle);
	if(hcdHandle->Instance==USB_OTG_FS)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USB_OTG_FS_CLK_DISABLE();

		/**USB_OTG_FS GPIO Configuration
		PA11     ------> USB_OTG_FS_DM
		PA12     ------> USB_OTG_FS_DP
		*/
		HAL_GPIO_DeInit(GPIOA, USB_DM_Pin | USB_DP_Pin);

		/* Peripheral interrupt Deinit*/
		HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
		DBG_V("Pins USB_DM and USB_DP Disabled\r\n");
	} else {
		DBG_E("hcdHandle->Instance NOT USB_OTG_FS\r\n");
	}
	DBG_N("Exit\r\n");

}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
	USBH_LL_IncTimer(hhcd->pData);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	DBG_N("Called within IRQ\n\r");
	USBH_LL_Connect(hhcd->pData);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	USBH_LL_Disconnect(hhcd->pData);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state)
{
	/* To be used with OS to sync URB state with the global state machine */
#if (USBH_USE_OS == 1)
	USBH_LL_NotifyURBChange(hhcd->pData);
#endif
}

/*******************************************************************************
                       LL Driver Interface (USB Host Library --> HCD)
*******************************************************************************/

USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost)
{
	/* Init USB_IP */
	DBG_N("Enter with phost %p\r\n", phost);
	if (phost->id == HOST_FS)
	{
		DBG_I("Linking the host driver to the stack\r\n");
		/* Link the driver to the stack. */
		hhcd_USB_OTG_FS.pData = phost;
		phost->pData = &hhcd_USB_OTG_FS;

		hhcd_USB_OTG_FS.Instance = USB_OTG_FS;
		hhcd_USB_OTG_FS.Init.Host_channels = 8;
		hhcd_USB_OTG_FS.Init.speed = HCD_SPEED_FULL;
		hhcd_USB_OTG_FS.Init.dma_enable = DISABLE;
		hhcd_USB_OTG_FS.Init.phy_itface = HCD_PHY_EMBEDDED;
		hhcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
		if (HAL_HCD_Init(&hhcd_USB_OTG_FS) != HAL_OK)
		{
			_Error_Handler(__FILE__, __LINE__);
		}
		USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_OTG_FS));
	} else {
		DBG_E("Cannot Link the host driver to the stack. "
			"Not HOST_FS ---> 0x%08x\r\n", phost->id);
	}
	return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with phost %p\r\n", phost);
	hal_status = HAL_HCD_DeInit(phost->pData);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with phost %p\r\n", phost);

	hal_status = HAL_HCD_Start(phost->pData);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with phost %p\r\n", phost);

	hal_status = HAL_HCD_Stop(phost->pData);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost)
{
	USBH_SpeedTypeDef speed = USBH_SPEED_FULL;

	DBG_N("Enter with phost %p\r\n", phost);

	switch (HAL_HCD_GetCurrentSpeed(phost->pData))
	{
		case 0 :
			speed = USBH_SPEED_HIGH;
			break;
		case 1 :
			speed = USBH_SPEED_FULL;
			break;
		case 2 :
			speed = USBH_SPEED_LOW;
			break;
		default:
			speed = USBH_SPEED_FULL;
			break;
	}
	return  speed;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with phost %p\r\n", phost);

	hal_status = HAL_HCD_ResetPort(phost->pData);
	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost, uint8_t pipe_num, uint8_t epnum,
                                    uint8_t dev_address, uint8_t speed, uint8_t ep_type, uint16_t mps)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with:\r\n"
		"\tphost %p - Pipe_num %d - Epnum %d\r\n"
		"\tdev_address %d - Speed %d\r\n"
		"\tep_type %d - mps %d\r\n", phost, pipe_num, epnum, dev_address,
			speed, ep_type, mps);

	hal_status = HAL_HCD_HC_Init(phost->pData, pipe_num, epnum,
		dev_address, speed, ep_type, mps);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with phost %p - pipe %d\r\n", phost, pipe);

	hal_status = HAL_HCD_HC_Halt(phost->pData, pipe);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_ERROR :
			usb_status = USBH_FAIL;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t direction,
                                     uint8_t ep_type, uint8_t token, uint8_t *pbuff, uint16_t length,
                                     uint8_t do_ping)
{
	HAL_StatusTypeDef hal_status = HAL_OK;
	USBH_StatusTypeDef usb_status = USBH_OK;

	DBG_N("Enter with:\r\n"
		"\tphost %p - pipe %d - direction %d\r\n"
		"\tep_type %d - token %d - pbuffer %p\r\n"
		"\tlength %d - do_ping %d\r\n", phost, pipe, direction, ep_type,
			token, pbuff, length, do_ping);

	hal_status = HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction ,
		ep_type, token, pbuff, length, do_ping);

	switch (hal_status)
	{
		case HAL_OK :
			usb_status = USBH_OK;
			break;
		case HAL_BUSY :
			usb_status = USBH_BUSY;
			break;
		case HAL_ERROR :
		case HAL_TIMEOUT :
		default :
			usb_status = USBH_FAIL;
			break;
	}
	return usb_status;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe)
{
  return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState (phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state)
{
	DBG_N("Enter with phost %p - state %d\r\n", phost, state);

	if (phost->id == HOST_FS)
	{
		DBG_N("Phost->id is HOST_FS\n\r");
		if (state == 0)
		{
			/* Drive high Charge pump */
			/* ToDo: Add IOE driver control */
		}
		else
		{
			/* Drive low Charge pump */
			/* ToDo: Add IOE driver control */
		}
	}
	else
	{
		DBG_E("Phost->id is NOT HOST_FS --> 0x%08x\n\r", phost->id);
	}
	HAL_Delay(200);
	DBG_N("Exit\r\n");
	return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle)
{
	HCD_HandleTypeDef *pHandle;
	pHandle = phost->pData;

	DBG_N("Enter with phost %p - pipe %d - toggle %d\r\n",
		phost, pipe, toggle);

	if(pHandle->hc[pipe].ep_is_in)
	{
		pHandle->hc[pipe].toggle_in = toggle;
	}
	else
	{
		pHandle->hc[pipe].toggle_out = toggle;
	}

	return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe)
{
	uint8_t toggle = 0;
	HCD_HandleTypeDef *pHandle;
	pHandle = phost->pData;

	DBG_N("Enter with phost %p - pipe %d\r\n", phost, pipe);
	if(pHandle->hc[pipe].ep_is_in)
	{
		toggle = pHandle->hc[pipe].toggle_in;
	}
	else
	{
		toggle = pHandle->hc[pipe].toggle_out;
	}
	return toggle;
}

void USBH_Delay(uint32_t Delay)
{
	HAL_Delay(Delay);
}

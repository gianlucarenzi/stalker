/**
  ******************************************************************************
  * @file            : usb_host.c
  * @version         : v1.0_Cube
  * @brief           : This file implements the USB Host
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

#include "debug.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"

static int debuglevel = DBG_INFO;

/* USB Host core handle declaration */
static USBH_HandleTypeDef hUsbHostFS;
static ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * user callback declaration
 */
void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/**
  * Init USB host library, add supported class and start the library
  * @retval None
  */
void MX_USB_HOST_Init(void)
{
	DBG_N("Called\n\r");
	/* Init host Library, add supported class and start the library. */
	USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);

	USBH_RegisterClass(&hUsbHostFS, USBH_HID_CLASS);

	USBH_Start(&hUsbHostFS);

}

/*
 * Background task
 */
void MX_USB_HOST_Process(void)
{
	/* USB Host Background task */
	USBH_Process(&hUsbHostFS);
}
/*
 * user callback definition
 */
void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
	DBG_N("Enter with: phost %p - id: %d\r\n", phost, id);

	switch(id)
	{
		default:
			DBG_N("ID: %d Application IDLE\r\n", id);
			Appli_state = APPLICATION_IDLE;
			break;

		case HOST_USER_DISCONNECTION:
			DBG_N("HOST_USER_DISCONNECTION: Application DISCONNECT\r\n");
			Appli_state = APPLICATION_DISCONNECT;
			break;

		case HOST_USER_SELECT_CONFIGURATION:
			DBG_N("HOST_USER_SELECT_CONFIGURATION: Application DISCONNECT\r\n");
			Appli_state = APPLICATION_DISCONNECT;
			break;

		case HOST_USER_CLASS_ACTIVE:
			DBG_N("HOST_USER_CLASS_ACTIVE: Application READY\r\n");
			Appli_state = APPLICATION_READY;
			break;

		case HOST_USER_CONNECTION:
			DBG_N("HOST_USER_CONNECTION: Application START\r\n");
			Appli_state = APPLICATION_START;
			break;
	}
}

ApplicationTypeDef USBH_ApplicationState(void)
{
	return Appli_state;
}

USBH_HandleTypeDef * USBH_GetHost(void)
{
	return &hUsbHostFS;
}

/**
	typedef struct
	{
	  uint8_t state;
	  uint8_t lctrl;
	  uint8_t lshift;
	  uint8_t lalt;
	  uint8_t lgui;
	  uint8_t rctrl;
	  uint8_t rshift;
	  uint8_t ralt;
	  uint8_t rgui;
	  uint8_t keys[6]; <--- Va convertito con la lookup table AMIGA
	}
	HID_KEYBD_Info_TypeDef;


 **/
static keyboard_code_t keycode;

int USBH_Keybd(USBH_HandleTypeDef *phost)
{
	HID_KEYBD_Info_TypeDef *k_pinfo;

	DBG_N("Enter with phost %p\r\n", phost);

	k_pinfo = USBH_HID_GetKeybdInfo(phost);

	if (k_pinfo != NULL)
	{
		int i;
		DBG_V(  "lctrl = %d lshift = %d lalt   = %d\r\n"
				"lgui  = %d rctrl  = %d rshift = %d\r\n"
				"ralt  = %d rgui   = %d\r\n",
				k_pinfo->lctrl, k_pinfo->lshift, k_pinfo->lalt,
				k_pinfo->lgui, k_pinfo->rctrl, k_pinfo->rshift,
				k_pinfo->ralt, k_pinfo->rgui);
		keycode.lctrl = k_pinfo->lctrl;
		keycode.lshift = k_pinfo->lshift;
		keycode.lalt = k_pinfo->lalt;
		keycode.lgui = k_pinfo->lgui;
		keycode.rctrl = k_pinfo->rctrl;
		keycode.rshift = k_pinfo->rshift;
		keycode.ralt = k_pinfo->ralt;
		keycode.rgui = k_pinfo->rgui;
		for (i = 0; i < KEY_PRESSED_MAX; i++)
		{
			if (debuglevel >= DBG_VERBOSE) {
				printf("--- KEY: %d -- KeyCODE: 0x%02x\r\n", i, k_pinfo->keys[i]);
			}
			keycode.keys[i] = k_pinfo->keys[i];
		}
		if (debuglevel >= DBG_VERBOSE) printf("\n\r");
		return 0;
	}
	else
	{
		return -1; // Device NOT READY
	}
}

keyboard_code_t * USBH_GetScanCode(void)
{
	return &keycode;
}

/**
  ******************************************************************************
  * @file    usbh_hid.c
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file is the HID Layer Handlers for USB Host HID class.
  *
  * @verbatim
  *      
  *          ===================================================================      
  *                                HID Class  Description
  *          =================================================================== 
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse and Keyboard protocols
  *      
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbh_hid.h"
#include "usbh_hid_parser.h"

extern volatile uint32_t usb_irq_count;
extern volatile uint32_t hc_in_irq_count[16];


/** @addtogroup USBH_LIB
* @{
*/

/** @addtogroup USBH_CLASS
* @{
*/

/** @addtogroup USBH_HID_CLASS
* @{
*/

/** @defgroup USBH_HID_CORE 
* @brief    This file includes HID Layer Handlers for USB Host HID class.
* @{
*/ 

/** @defgroup USBH_HID_CORE_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Defines
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Macros
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Variables
* @{
*/

/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_FunctionPrototypes
* @{
*/ 

static USBH_StatusTypeDef USBH_HID_InterfaceInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_InterfaceDeInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_HID_SOFProcess(USBH_HandleTypeDef *phost);
static void  USBH_HID_ParseHIDDesc (HID_DescTypeDef *desc, uint8_t *buf);

extern USBH_StatusTypeDef USBH_HID_MouseInit(USBH_HandleTypeDef *phost);
extern USBH_StatusTypeDef USBH_HID_KeybdInit(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef  HID_Class = 
{
  "HID",
  USB_HID_CLASS,
  USBH_HID_InterfaceInit,
  USBH_HID_InterfaceDeInit,
  USBH_HID_ClassRequest,
  USBH_HID_Process,
  USBH_HID_SOFProcess,
  NULL,
};
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Functions
* @{
*/ 


/**
  * @brief  USBH_HID_InterfaceInit 
  *         The function init the HID class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_InterfaceInit (USBH_HandleTypeDef *phost)
{	
  uint8_t max_ep;
  uint8_t num = 0;
  uint8_t interface;
#ifdef DEBUG_USB
  USBH_UsrLog("USBH_HID_InterfaceInit\r\n");
  USBH_UsrLog("  VID/PID: %04Xh/%04Xh\r\n", phost->device.DevDesc.idVendor, phost->device.DevDesc.idProduct);
#endif
  
  USBH_StatusTypeDef status = USBH_FAIL ;
  HID_HandleTypeDef *HID_Handle;
  
  USBH_UsrLog("USBH_HID_InterfaceInit: Number of Interfaces = %d\r\n", phost->device.CfgDesc.bNumInterfaces);
  for (uint8_t i = 0; i < phost->device.CfgDesc.bNumInterfaces; i++) {
      USBH_UsrLog("  Interface %d: Class=%02Xh, SubClass=%02Xh, Protocol=%02Xh, NumEndpoints=%d\r\n",
                  i,
                  phost->device.CfgDesc.Itf_Desc[i].bInterfaceClass,
                  phost->device.CfgDesc.Itf_Desc[i].bInterfaceSubClass,
                  phost->device.CfgDesc.Itf_Desc[i].bInterfaceProtocol,
                  phost->device.CfgDesc.Itf_Desc[i].bNumEndpoints);
      for (uint8_t j = 0; j < phost->device.CfgDesc.Itf_Desc[i].bNumEndpoints; j++) {
          USBH_UsrLog("    Endpoint %d: Address=%02Xh, Type=%02Xh, MaxPacketSize=%d\r\n",
                      j,
                      phost->device.CfgDesc.Itf_Desc[i].Ep_Desc[j].bEndpointAddress,
                      phost->device.CfgDesc.Itf_Desc[i].Ep_Desc[j].bmAttributes & 0x03,
                      phost->device.CfgDesc.Itf_Desc[i].Ep_Desc[j].wMaxPacketSize);
      }
  }

  interface = USBH_FindInterface(phost, phost->pActiveClass->ClassCode, HID_BOOT_CODE, 0xFF);
  
  if(interface == 0xFF) /* No Valid Interface */
  {
    status = USBH_FAIL;  
    USBH_DbgLog ("Cannot Find the interface for %s class.\r\n", phost->pActiveClass->Name);         
  }
  else
  {
    USBH_SelectInterface (phost, interface);
    phost->pActiveClass->pData = (HID_HandleTypeDef *)USBH_malloc (sizeof(HID_HandleTypeDef));
    HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData; 
    HID_Handle->state = HID_ERROR;
    
    /*Decode Bootclass Protocol: Mouse or Keyboard*/
    if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol == HID_KEYBRD_BOOT_CODE)
    {
#ifdef DEBUG_USB
      USBH_UsrLog("  Protocol: Keyboard\r\n");
#endif
      USBH_UsrLog ("KeyBoard device found!\r\n");
      USBH_Delay(50);  // Give log queue time to flush
      USBH_UsrLog ("DEBUG: After 'KeyBoard device found!' message\r\n");
      HID_Handle->Init =  USBH_HID_KeybdInit;
      USBH_Delay(50);  // Give log queue time to flush
      USBH_UsrLog ("DEBUG: HID_Handle->Init set to USBH_HID_KeybdInit (%p)\r\n", (void*)HID_Handle->Init);
    }
    else if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol  == HID_MOUSE_BOOT_CODE)		  
    {
#ifdef DEBUG_USB
      USBH_UsrLog("  Protocol: Mouse\r\n");
#endif
      USBH_UsrLog ("Mouse device found!\r\n");         
      HID_Handle->Init =  USBH_HID_MouseInit;     
    }
    else
    {
#ifdef DEBUG_USB
      USBH_UsrLog("  Protocol: Unsupported (%d)\r\n", phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol);
#endif
      USBH_UsrLog ("Protocol not supported.\r\n");  
      return USBH_FAIL;
    }
    
    USBH_Delay(100);  // Give log queue time to flush
    USBH_UsrLog ("DEBUG: Setting HID_Handle->state = HID_INIT\r\n");
    HID_Handle->state     = HID_INIT;
    HID_Handle->ctl_state = HID_REQ_INIT;
    USBH_Delay(50);
    USBH_UsrLog ("DEBUG: Reading endpoint descriptor...\r\n");
    HID_Handle->ep_addr   = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress;
    HID_Handle->length    = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize;
    HID_Handle->poll      = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bInterval ;

#ifdef DEBUG_USB
    USBH_UsrLog("  Endpoint address: %02Xh\r\n", HID_Handle->ep_addr);
    USBH_UsrLog("  Max packet size: %d bytes\r\n", HID_Handle->length);
    USBH_UsrLog("  Polling interval: %d ms\r\n", HID_Handle->poll);
#endif
    USBH_UsrLog ("DEBUG: Endpoint info logged\r\n");
    
    if (HID_Handle->poll  < HID_MIN_POLL) 
    {
      HID_Handle->poll = HID_MIN_POLL;
    }
    
    /* Check fo available number of endpoints */
    /* Find the number of EPs in the Interface Descriptor */      
    /* Choose the lower number in order not to overrun the buffer allocated */
    max_ep = ( (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ? 
              phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bNumEndpoints :
                  USBH_MAX_NUM_ENDPOINTS); 
    
    
    /* Decode endpoint IN and OUT address from interface descriptor */
    for ( ;num < max_ep; num++)
    {
      if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress & 0x80)
      {
        HID_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
        HID_Handle->InPipe  =\
          USBH_AllocPipe(phost, HID_Handle->InEp);
        
        /* Open pipe for IN endpoint */
        USBH_OpenPipe  (phost,
                        HID_Handle->InPipe,
                        HID_Handle->InEp,
                        phost->device.address,
                        phost->device.speed,
                        USB_EP_TYPE_INTR,
                        HID_Handle->length); 
        
        USBH_LL_SetToggle (phost, HID_Handle->InPipe, 0);
        
      }
      else
      {
        HID_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[num].bEndpointAddress);
        HID_Handle->OutPipe  =\
          USBH_AllocPipe(phost, HID_Handle->OutEp);
        
        /* Open pipe for OUT endpoint */
        USBH_OpenPipe  (phost,
                        HID_Handle->OutPipe,
                        HID_Handle->OutEp,                            
                        phost->device.address,
                        phost->device.speed,
                        USB_EP_TYPE_INTR,
                        HID_Handle->length); 
        
        USBH_LL_SetToggle (phost, HID_Handle->OutPipe, 0);        
      }
      
    }

    USBH_UsrLog("USBH_HID_InterfaceInit: About to set status to USBH_OK\r\n");
    USBH_UsrLog("USBH_HID_InterfaceInit: HID_Handle->Init function pointer = %p\r\n", (void*)HID_Handle->Init);

    /* Workaround: Call Init function directly here since USBH_HID_Process may not be called at the right time */
    if (HID_Handle->Init != NULL) {
        USBH_UsrLog("USBH_HID_InterfaceInit: Calling Init function directly (workaround)\r\n");
        HID_Handle->Init(phost);
    } else {
        USBH_UsrLog("USBH_HID_InterfaceInit: ERROR - Init function pointer is NULL!\r\n");
    }

    status = USBH_OK;
  }
  return status;
}

/**
  * @brief  USBH_HID_InterfaceDeInit 
  *         The function DeInit the Pipes used for the HID class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_InterfaceDeInit (USBH_HandleTypeDef *phost )
{	
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData; 
  
  if(HID_Handle->InPipe != 0x00)
  {   
    USBH_ClosePipe  (phost, HID_Handle->InPipe);
    USBH_FreePipe  (phost, HID_Handle->InPipe);
    HID_Handle->InPipe = 0;     /* Reset the pipe as Free */  
  }
  
  if(HID_Handle->OutPipe != 0x00)
  {   
    USBH_ClosePipe(phost, HID_Handle->OutPipe);
    USBH_FreePipe  (phost, HID_Handle->OutPipe);
    HID_Handle->OutPipe = 0;     /* Reset the pipe as Free */  
  }
  
  if(phost->pActiveClass->pData)
  {
    USBH_free (phost->pActiveClass->pData);
  }

  return USBH_OK;
}

/**
  * @brief  USBH_HID_ClassRequest 
  *         The function is responsible for handling Standard requests
  *         for HID class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_ClassRequest(USBH_HandleTypeDef *phost)
{   
  
  USBH_StatusTypeDef status         = USBH_BUSY;
  USBH_StatusTypeDef classReqStatus = USBH_BUSY;
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData; 

  /* Switch HID state machine */
  switch (HID_Handle->ctl_state)
  {
  case HID_REQ_INIT:  
  case HID_REQ_GET_HID_DESC:
    
    /* Get HID Desc */ 
    if (USBH_HID_GetHIDDescriptor (phost, USB_HID_DESC_SIZE)== USBH_OK)
    {
      
      USBH_HID_ParseHIDDesc(&HID_Handle->HID_Desc, phost->device.Data);
      USBH_UsrLog("USBH_HID_ClassRequest: HID Report Descriptor Length (wItemLength) = %d\r\n", HID_Handle->HID_Desc.wItemLength);
      HID_Handle->ctl_state = HID_REQ_GET_REPORT_DESC;
    }
    
    break;     
  case HID_REQ_GET_REPORT_DESC:
    
    
    /* Get Report Desc */ 
    if (USBH_HID_GetHIDReportDescriptor(phost, HID_Handle->HID_Desc.wItemLength) == USBH_OK)
    {
      /* The descriptor is available in phost->device.Data */

      HID_Handle->ctl_state = HID_REQ_SET_IDLE;
    }
    
    break;
    
  case HID_REQ_SET_IDLE:
    
    classReqStatus = USBH_HID_SetIdle (phost, 0, 0);
    
    /* set Idle */
    if (classReqStatus == USBH_OK)
    {
      HID_Handle->ctl_state = HID_REQ_SET_PROTOCOL;  
    }
    else if(classReqStatus == USBH_NOT_SUPPORTED) 
    {
      HID_Handle->ctl_state = HID_REQ_SET_PROTOCOL;        
    } 
    break; 
    
  case HID_REQ_SET_PROTOCOL:
    /* set protocol */
    if (USBH_HID_SetProtocol (phost, 0) == USBH_OK)
    {
      HID_Handle->ctl_state = HID_REQ_IDLE;
      
      /* all requests performed*/
      phost->pUser(phost, HOST_USER_CLASS_ACTIVE); 
      status = USBH_OK; 
    } 
    break;
    
  case HID_REQ_IDLE:
  default:
    break;
  }
  
  return status; 
}

/**
  * @brief  USBH_HID_Process 
  *         The function is for managing state machine for HID data transfers 
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_Process(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_OK;
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
    static uint8_t pcount = 0;

#ifdef DEBUG_USB
  static uint32_t process_call_count = 0;
  static uint8_t last_state = 0xFF;

  process_call_count++;
  if ((process_call_count % 1000) == 0) {
    USBH_DbgLog("USBH_HID_Process called %lu times, state=%d\r\n", process_call_count, HID_Handle->state);
  }

  if (HID_Handle->state != last_state) {
    USBH_DbgLog("HID state: %d\r\n", HID_Handle->state);
    last_state = HID_Handle->state;
  }
#endif

  switch (HID_Handle->state)
  {
  case HID_INIT:
    USBH_UsrLog("USBH_HID_Process: HID_INIT case - calling HID_Handle->Init(phost)\r\n");
    USBH_UsrLog("USBH_HID_Process: HID_Handle->Init function pointer = %p\r\n", (void*)HID_Handle->Init);
    if (HID_Handle->Init != NULL) {
        HID_Handle->Init(phost);
    } else {
        USBH_UsrLog("USBH_HID_Process: ERROR - Init function pointer is NULL in HID_INIT!\r\n");
    }
    break;

  case HID_IDLE:
    if(USBH_HID_GetReport (phost,
                           0x01,
                            0,
                            HID_Handle->pData,
                            HID_Handle->length) == USBH_OK)
    {
      
      fifo_write(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length);  
      HID_Handle->state = HID_SYNC;
    }
    
    break;
    
  case HID_SYNC:

    /* Sync with start of Even Frame */
    if(phost->Timer & 1)
    {
      HID_Handle->state = HID_GET_DATA; 
    }
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
#endif   
    break;
    
  case HID_GET_DATA:
  {
    USBH_StatusTypeDef status;
#ifdef DEBUG_USB
    if (pcount++ < 20) USBH_DbgLog("HID_GET_DATA: Starting interrupt receive (length=%d, pipe=%d)\r\n", HID_Handle->length, HID_Handle->InPipe);
#endif
    status = USBH_InterruptReceiveData(phost,
                              HID_Handle->pData,
                              HID_Handle->length,
                              HID_Handle->InPipe);

#ifdef DEBUG_USB
    if (pcount <= 20) USBH_DbgLog("HID_GET_DATA: USBH_InterruptReceiveData returned %d\r\n", status);
#endif

    HID_Handle->state = HID_POLL;
    HID_Handle->timer = phost->Timer;
    HID_Handle->DataReady = 0;
    break;
  }
    
  case HID_POLL:
  {
    USBH_URBStateTypeDef urb_state = USBH_LL_GetURBState(phost, HID_Handle->InPipe);
#ifdef DEBUG_USB
    static uint32_t poll_check_count = 0;
    static USBH_URBStateTypeDef last_urb_state = 0xFF;
    poll_check_count++;

    /* Log when URB state changes or every 100 checks (more frequent) */
    if (urb_state != last_urb_state || (poll_check_count % 500) == 0) {
      HCD_HandleTypeDef *pHCD = (HCD_HandleTypeDef *)phost->pData;
      uint8_t ch_num = HID_Handle->InPipe;
      USBH_DbgLog("HID_POLL: URB=%d check#%lu USB_IRQ=%lu CH_IRQ[%d]=%lu CH_STATE=%d\r\n",
                  urb_state, poll_check_count, usb_irq_count, ch_num, hc_in_irq_count[ch_num], pHCD->hc[ch_num].state);
      last_urb_state = urb_state;
    }
#endif

    if(urb_state == USBH_URB_DONE)
    {
#ifdef DEBUG_USB
      USBH_DbgLog("HID_POLL: URB DONE, writing to FIFO\r\n");
#endif
      if(HID_Handle->DataReady == 0)
      {
		fifo_write(&HID_Handle->fifo, HID_Handle->pData, HID_Handle->length);
		HID_Handle->DataReady = 1;
		USBH_HID_EventCallback(phost);
#if (USBH_USE_OS == 1)
		osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
#endif
      }
	}
    else if(urb_state == USBH_URB_STALL) /* IN Endpoint Stalled */
    {
#ifdef DEBUG_USB
      USBH_DbgLog("HID_POLL: URB STALL detected\r\n");
#endif

      /* Issue Clear Feature on interrupt IN endpoint */
      if(USBH_ClrFeature(phost,
                         HID_Handle->ep_addr) == USBH_OK)
      {
        /* Change state to issue next IN token */
        HID_Handle->state = HID_GET_DATA;
      }
    }
    else if(urb_state == USBH_URB_NOTREADY) /* NAK received, device not ready */
    {
#ifdef DEBUG_USB
      static uint32_t nak_log_count = 0;
      if ((nak_log_count++ % 500) == 0) {
        USBH_DbgLog("HID_POLL: URB NOTREADY (NAK), retrying transfer\r\n");
      }
#endif
      /* Go back to GET_DATA to restart the transfer */
      HID_Handle->state = HID_GET_DATA;
    }


    break;
  }
    
  default:
    break;
  }
  return status;
}

/**
  * @brief  USBH_HID_SOFProcess 
  *         The function is for managing the SOF Process 
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HID_SOFProcess(USBH_HandleTypeDef *phost)
{
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData;

#ifdef DEBUG_USB
  static uint32_t sof_call_count = 0;
  sof_call_count++;
  if ((sof_call_count % 1000) == 0) {
    USBH_DbgLog("SOF called %lu times, HID state=%d\r\n", sof_call_count, HID_Handle->state);
  }
#endif

  if(HID_Handle->state == HID_POLL)
  {
    if(( phost->Timer - HID_Handle->timer) >= HID_Handle->poll)
    {
#ifdef DEBUG_USB
     static uint8_t pcount = 0;
     if (pcount++ < 20) USBH_DbgLog("SOF: Poll interval elapsed, moving to HID_GET_DATA\r\n");
#endif
      HID_Handle->state = HID_GET_DATA;
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
#endif
    }
  }
  return USBH_OK;
}

/**
* @brief  USBH_Get_HID_ReportDescriptor
  *         Issue report Descriptor command to the device. Once the response 
  *         received, parse the report descriptor and update the status.
  * @param  phost: Host handle
  * @param  Length : HID Report Descriptor Length
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_GetHIDReportDescriptor (USBH_HandleTypeDef *phost,
                                                         uint16_t length)
{
  
  USBH_StatusTypeDef status;
  
  status = USBH_GetDescriptor(phost,
                              USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD,                                  
                              USB_DESC_HID_REPORT, 
                              phost->device.Data,
                              length);
  
  /* HID report descriptor is available in phost->device.Data.
  In case of USB Boot Mode devices for In report handling ,
  HID report descriptor parsing is not required.
  In case, for supporting Non-Boot Protocol devices and output reports,
  user may parse the report descriptor*/
  
  
  return status;
}

/**
  * @brief  USBH_Get_HID_Descriptor
  *         Issue HID Descriptor command to the device. Once the response 
  *         received, parse the report descriptor and update the status.
  * @param  phost: Host handle
  * @param  Length : HID Descriptor Length
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_GetHIDDescriptor (USBH_HandleTypeDef *phost,
                                            uint16_t length)
{
  
  USBH_StatusTypeDef status;
  
  status = USBH_GetDescriptor( phost,
                              USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD,                                  
                              USB_DESC_HID,
                              phost->device.Data,
                              length);
 
  return status;
}

/**
  * @brief  USBH_Set_Idle
  *         Set Idle State. 
  * @param  phost: Host handle
  * @param  duration: Duration for HID Idle request
  * @param  reportId : Targeted report ID for Set Idle request
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetIdle (USBH_HandleTypeDef *phost,
                                         uint8_t duration,
                                         uint8_t reportId)
{
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_IDLE;
  phost->Control.setup.b.wValue.w = (duration << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(phost, 0 , 0 );
}


/**
  * @brief  USBH_HID_Set_Report
  *         Issues Set Report 
  * @param  phost: Host handle
  * @param  reportType  : Report type to be sent
  * @param  reportId    : Targeted report ID for Set Report request
  * @param  reportBuff  : Report Buffer
  * @param  reportLen   : Length of data report to be send
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetReport (USBH_HandleTypeDef *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t* reportBuff,
                                    uint8_t reportLen)
{
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;
  
  return USBH_CtlReq(phost, reportBuff , reportLen );
}


/**
  * @brief  USBH_HID_GetReport
  *         retreive Set Report 
  * @param  phost: Host handle
  * @param  reportType  : Report type to be sent
  * @param  reportId    : Targeted report ID for Set Report request
  * @param  reportBuff  : Report Buffer
  * @param  reportLen   : Length of data report to be send
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_GetReport (USBH_HandleTypeDef *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t* reportBuff,
                                    uint8_t reportLen)
{
  
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_GET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;
  
  return USBH_CtlReq(phost, reportBuff , reportLen );
}

/**
  * @brief  USBH_Set_Protocol
  *         Set protocol State.
  * @param  phost: Host handle
  * @param  protocol : Set Protocol for HID : boot/report protocol
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_HID_SetProtocol(USBH_HandleTypeDef *phost,
                                            uint8_t protocol)
{
  
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_PROTOCOL;
  phost->Control.setup.b.wValue.w = protocol;
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(phost, 0 , 0 );
  
}

/**
  * @brief  USBH_ParseHIDDesc 
  *         This function Parse the HID descriptor
  * @param  desc: HID Descriptor
  * @param  buf: Buffer where the source descriptor is available
  * @retval None
  */
static void  USBH_HID_ParseHIDDesc (HID_DescTypeDef *desc, uint8_t *buf)
{
  
  desc->bLength                  = *(uint8_t  *) (buf + 0);
  desc->bDescriptorType          = *(uint8_t  *) (buf + 1);
  desc->bcdHID                   =  LE16  (buf + 2);
  desc->bCountryCode             = *(uint8_t  *) (buf + 4);
  desc->bNumDescriptors          = *(uint8_t  *) (buf + 5);
  desc->bReportDescriptorType    = *(uint8_t  *) (buf + 6);
  desc->wItemLength              =  LE16  (buf + 7);
} 

/**
  * @brief  USBH_HID_GetDeviceType
  *         Return Device function.
  * @param  phost: Host handle
  * @retval HID function: HID_MOUSE / HID_KEYBOARD
  */
HID_TypeTypeDef USBH_HID_GetDeviceType(USBH_HandleTypeDef *phost)
{
  HID_TypeTypeDef   type = HID_UNKNOWN;
  
  if(phost->gState == HOST_CLASS)
  {
    
    if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol \
      == HID_KEYBRD_BOOT_CODE)
    {
      type = HID_KEYBOARD;  
    }
    else if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].bInterfaceProtocol \
      == HID_MOUSE_BOOT_CODE)		  
    {
      type=  HID_MOUSE;  
    }
  }
  return type;
}


/**
  * @brief  USBH_HID_GetPollInterval
  *         Return HID device poll time
  * @param  phost: Host handle
  * @retval poll time (ms)
  */
uint8_t USBH_HID_GetPollInterval(USBH_HandleTypeDef *phost)
{
  HID_HandleTypeDef *HID_Handle =  (HID_HandleTypeDef *) phost->pActiveClass->pData;
    
    if((phost->gState == HOST_CLASS_REQUEST) ||
       (phost->gState == HOST_INPUT) ||
         (phost->gState == HOST_SET_CONFIGURATION) ||
           (phost->gState == HOST_CHECK_CLASS) ||           
             ((phost->gState == HOST_CLASS)))
  {
    return (HID_Handle->poll);
  }
  else
  {
    return 0;
  }
}
/**
  * @brief  fifo_init
  *         Initialize FIFO.
  * @param  f: Fifo address
  * @param  buf: Fifo buffer
  * @param  size: Fifo Size
  * @retval none
  */
void fifo_init(FIFO_TypeDef * f, uint8_t * buf, uint16_t size)
{
     f->head = 0;
     f->tail = 0;
     f->lock = 0;
     f->size = size;
     f->buf = buf;
}

/**
  * @brief  fifo_read
  *         Read from FIFO.
  * @param  f: Fifo address
  * @param  buf: read buffer 
  * @param  nbytes: number of item to read
  * @retval number of read items
  */
uint16_t  fifo_read(FIFO_TypeDef * f, void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  uint8_t * p;
  p = (uint8_t*) buf;
  
  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( f->tail != f->head )
      { 
        *p++ = f->buf[f->tail];  
        f->tail++;  
        if( f->tail == f->size )
        {  
          f->tail = 0;
        }
      } else 
      {
        f->lock = 0;
        return i; 
      }
    }
  }
  f->lock = 0;
  return nbytes;
}
 
/**
  * @brief  fifo_write
  *         Read from FIFO.
  * @param  f: Fifo address
  * @param  buf: read buffer 
  * @param  nbytes: number of item to write
  * @retval number of written items
  */
uint16_t  fifo_write(FIFO_TypeDef * f, const void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  const uint8_t * p;
  p = (const uint8_t*) buf;
  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( (f->head + 1 == f->tail) ||
         ( (f->head + 1 == f->size) && (f->tail == 0)) )
      {
        f->lock = 0;
        return i;
      } 
      else 
      {
        f->buf[f->head] = *p++;
        f->head++;
        if( f->head == f->size )
        {
          f->head = 0;
        }
      }
    }
	f->lock = 0;
    return nbytes;
  }
  return 0;
}



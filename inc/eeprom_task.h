/*
 * eeprom_task.h
 *
 *  Created on: Nov 12, 2025
 *      Author: Gianluca Renzi
 */

#ifndef INC_EEPROM_TASK_H_
#define INC_EEPROM_TASK_H_

#include "cmsis_os.h"
#include <stdint.h>

// Definisce gli indirizzi virtuali per le variabili in EEPROM
#define VIRTUAL_ADDR_SYSTEM_MODE   0x0001

// Define return codes for EEPROM operations
#define EE_OK                   0
#define EE_NO_DATA              1 // Indicates that the virtual address has not been written yet



typedef enum {
    EEPROM_CMD_WRITE,
    EEPROM_CMD_READ,
} eeprom_cmd_t;

typedef struct {
    eeprom_cmd_t cmd;
    uint16_t virt_addr;
    uint16_t data;      // Data for write operations
    uint16_t *data_ptr; // Pointer for read operations
    osSemaphoreId_t sync_sem; // Semaphore for synchronous operations
    uint16_t *read_status_ptr; // Pointer to store the return status of the read operation
} eeprom_msg_t;

extern osMessageQueueId_t eepromQueueHandle;

/**
 * @brief Initializes the EEPROM task and its queue.
 * @retval None
 */
void eeprom_task_init(void);

/**
 * @brief Sends an asynchronous write request to the EEPROM task.
 * @param v_addr Virtual address to write to.
 * @param data 16-bit data to write.
 * @retval osStatus_t Status of the queue send operation.
 */
osStatus_t send_eeprom_write_request(uint16_t v_addr, uint16_t data);

/**
 * @brief Reads a variable from the EEPROM synchronously.
 * @param v_addr Virtual address to read from.
 * @param data Pointer to a 16-bit variable to store the read data.
 * @retval uint16_t The status from EE_ReadVariable (EE_OK, EE_NO_DATA, etc.), or 0xFFFF on a sync error.
 */
uint16_t eeprom_read_variable_sync(uint16_t v_addr, uint16_t *data);


#endif /* INC_EEPROM_TASK_H_ */

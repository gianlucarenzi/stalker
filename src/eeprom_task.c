#include "eeprom_task.h"
#include "main.h"
#include "debug.h"
#include "amiga.h"
#include <stdbool.h>
#include "ee.h"
#include "ee_config.h" // For EE_SELECTED_PAGE_SECTOR_SIZE

static int debuglevel = DBG_INFO;

// Buffer in RAM to mirror EEPROM content (e.g., 16 uint16_t variables = 32 bytes)
#define EEPROM_VIRTUAL_SIZE_BYTES 32
static uint8_t eeprom_ram_buffer[EEPROM_VIRTUAL_SIZE_BYTES];

osMessageQueueId_t eepromQueueHandle;
static void EepromTask(void *argument);

extern volatile reset_keypress_mode_t current_mode; // Definita in main.c

/**
 * @brief Inizializza e crea il task per la gestione della EEPROM e la relativa coda.
 */
void eeprom_task_init(void) {
	const osMessageQueueAttr_t queue_attributes = {
		.name = "eepromQueue"
	};
	eepromQueueHandle = osMessageQueueNew(16, sizeof(eeprom_msg_t), &queue_attributes);

	const osThreadAttr_t task_attributes = {
		.name = "eepromTask",
		.stack_size = 256 * 4,
		.priority = (osPriority_t) osPriorityNormal,
	};
	osThreadNew(EepromTask, NULL, &task_attributes);
}

/**
 * @brief Sends an asynchronous write request to the EEPROM task.
 */
osStatus_t send_eeprom_write_request(uint16_t v_addr, uint16_t data)
{
	eeprom_msg_t msg;
	msg.cmd = EEPROM_CMD_WRITE;
	msg.virt_addr = v_addr;
	msg.data = data;
	msg.data_ptr = NULL;
	msg.sync_sem = NULL;
	msg.read_status_ptr = NULL;
	return osMessageQueuePut(eepromQueueHandle, &msg, 0, 0);
}

uint16_t eeprom_read_variable_sync(uint16_t v_addr, uint16_t *data)
{
	osStatus_t status;
	eeprom_msg_t msg;
	uint16_t read_status = 0xFFFF; // Default to error

	// Create a binary semaphore for synchronization
	const osSemaphoreAttr_t sem_attr = { .name = "eepromReadSem" };
	osSemaphoreId_t sync_sem = osSemaphoreNew(1, 0, &sem_attr); // Max count 1, initial count 0
	if (sync_sem == NULL)
	{
		DBG_E("Failed to create semaphore for EEPROM read.\r\n");
		return read_status; // Return 0xFFFF
	}

	msg.cmd = EEPROM_CMD_READ;
	msg.virt_addr = v_addr;
	msg.data = 0;
	msg.data_ptr = data;
	msg.sync_sem = sync_sem;
	msg.read_status_ptr = &read_status;

	// Send the message to the EEPROM task
	status = osMessageQueuePut(eepromQueueHandle, &msg, 0, osWaitForever);
	if (status != osOK)
	{
		DBG_E("Failed to send EEPROM read request to queue.\r\n");
		osSemaphoreDelete(sync_sem);
		return read_status; // Return 0xFFFF
	}

	// Wait for the EEPROM task to complete the read and signal the semaphore
	status = osSemaphoreAcquire(sync_sem, osWaitForever);
	if (status != osOK)
	{
		DBG_E("Failed to acquire semaphore for EEPROM read.\r\n");
		osSemaphoreDelete(sync_sem);
		return read_status; // Return 0xFFFF
	}

	// Delete the semaphore as it's no longer needed
	osSemaphoreDelete(sync_sem);

	return read_status;
}

/**
 * @brief Task principale per la gestione della EEPROM.
 */
static void EepromTask(void *argument)
{
	DBG_N("EEPROM Task: Started.\r\n");

	// 1. Initialize the EEPROM emulation library
	if (!ee_init(eeprom_ram_buffer, EEPROM_VIRTUAL_SIZE_BYTES)) {
		DBG_E("EEPROM Task: EEPROM emulation init failed! Task will be suspended.\r\n");
		osThreadSuspend(osThreadGetId());
	}

	// 2. Read initial content from flash into RAM buffer
	ee_read();
	DBG_W("EEPROM Task: Initial EEPROM content read from flash.\r\n");

	eeprom_msg_t msg;
	osStatus_t status;

	// 3. Loop principale: attendi comandi di scrittura / lettura dalla coda
	for (;;)
	{
		DBG_N("EEPROM Task: Waiting for message.\r\n");
		status = osMessageQueueGet(eepromQueueHandle, &msg, NULL, osWaitForever);
		DBG_N("EEPROM Task: Message received, status: %d.\r\n", status);

		if (status == osOK)
		{
			switch (msg.cmd)
			{
				case EEPROM_CMD_WRITE:
					DBG_W("EEPROM Task: Processing WRITE command for addr 0x%04X with data 0x%04X.\r\n", msg.virt_addr, msg.data);
					// Write 16-bit data to the RAM buffer
					if ((msg.virt_addr * sizeof(uint16_t) + sizeof(uint16_t)) <= EEPROM_VIRTUAL_SIZE_BYTES)
					{
						((uint16_t*)eeprom_ram_buffer)[msg.virt_addr] = msg.data;
						if (ee_write())
						{
							DBG_W("EEPROM Task: WRITE command processed and committed to flash"
								" with value 0x%02x.\r\n", msg.data);
						}
						else
						{
							DBG_E("EEPROM Task: Failed to commit WRITE to flash for addr 0x%04X.\r\n", msg.virt_addr);
						}
					}
					else
					{
						DBG_E("EEPROM Task: WRITE address 0x%04X out of bounds.\r\n", msg.virt_addr);
					}
					break;
				case EEPROM_CMD_READ:
					DBG_W("EEPROM Task: Processing READ command for addr 0x%04X.\r\n", msg.virt_addr);
					if (msg.data_ptr != NULL && msg.read_status_ptr != NULL)
					{
						// Read 16-bit data from the RAM buffer
						if ((msg.virt_addr * sizeof(uint16_t) + sizeof(uint16_t)) <= EEPROM_VIRTUAL_SIZE_BYTES)
						{
							uint16_t value = ((uint16_t*)eeprom_ram_buffer)[msg.virt_addr];
							*msg.data_ptr = value;
							// Determine read status: if value is 0xFFFF, consider it as EE_NO_DATA
							*msg.read_status_ptr = (value == 0xFFFF) ? EE_NO_DATA : EE_OK;
							DBG_N("EEPROM Task: READ command processed. Value: 0x%04x, Status: %d.\r\n",
								value, *msg.read_status_ptr);
						}
						else
						{
							DBG_E("EEPROM Task: READ address 0x%04X out of bounds.\r\n", msg.virt_addr);
							*msg.read_status_ptr = 0xFFFF; // Indicate error
						}
					}
					if (msg.sync_sem != NULL)
					{
						osSemaphoreRelease(msg.sync_sem);
					}
					break;
				default:
					DBG_E("EEPROM Task: Received unknown command.\r\n");
					break;
			}
		}
	}
}

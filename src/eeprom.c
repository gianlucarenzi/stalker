#include "eeprom.h"
#include <string.h>

// Define the flash sector used for EEPROM emulation
#define EEPROM_SECTOR      FLASH_SECTOR_1
#define EEPROM_START_ADDRESS ((uint32_t)&_eeprom_start)

// RAM buffer for holding the EEPROM sector data
static uint32_t eeprom_buffer[EEPROM_SIZE / sizeof(uint32_t)];

/**
 * @brief  Reads a 32-bit word from the EEPROM emulation.
 * @param  address: The address offset within the EEPROM sector.
 * @param  data: Pointer to the variable to store the read data.
 * @retval HAL status.
 */
HAL_StatusTypeDef eeprom_read(uint32_t address, uint32_t *data)
{
    if (address >= EEPROM_SIZE)
    {
        return HAL_ERROR;
    }

    *data = *(__IO uint32_t *)(EEPROM_START_ADDRESS + address);

    return HAL_OK;
}

/**
 * @brief  Writes a 32-bit word to the EEPROM emulation using a buffered approach.
 * @param  address: The address offset within the EEPROM sector to write to.
 * @param  data: The 32-bit data to write.
 * @retval HAL status.
 * @note   This function is placed in RAM to allow execution while the flash is being written.
 */
__attribute__((section(".RamFunc"))) HAL_StatusTypeDef eeprom_write(uint32_t address, uint32_t data)
{
    if (address >= EEPROM_SIZE)
    {
        return HAL_ERROR;
    }

    // 1. Copy the entire EEPROM flash sector into the RAM buffer
    memcpy(eeprom_buffer, (void *)EEPROM_START_ADDRESS, EEPROM_SIZE);

    // 2. Modify the specific data word in the RAM buffer
    uint32_t buffer_index = address / sizeof(uint32_t);
    if (eeprom_buffer[buffer_index] == data) {
        // Data is the same, no need to write
        return HAL_OK;
    }
    eeprom_buffer[buffer_index] = data;

    // 3. Erase the EEPROM flash sector
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = EEPROM_SECTOR;
    EraseInitStruct.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    // 4. Write the entire RAM buffer back to the flash sector
    for (uint32_t i = 0; i < (EEPROM_SIZE / sizeof(uint32_t)); ++i)
    {
        uint32_t write_address = EEPROM_START_ADDRESS + (i * sizeof(uint32_t));
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_address, eeprom_buffer[i]);
        if (status != HAL_OK)
        {
            break; // Exit loop on first error
        }
    }

    HAL_FLASH_Lock();

    return status;
}
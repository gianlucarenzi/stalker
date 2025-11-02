#include "eeprom.h"

/**
 * @brief  Reads a 32-bit word from the EEPROM emulation.
 * @param  address: The address to read from.
 * @param  data: Pointer to the variable to store the read data.
 * @retval HAL status.
 */
HAL_StatusTypeDef eeprom_read(uint32_t address, uint32_t *data)
{
    if (address >= EEPROM_SIZE)
    {
        return HAL_ERROR;
    }

    *data = *(__IO uint32_t *)((uint32_t)&_eeprom_start + address);

    return HAL_OK;
}

/**
 * @brief  Writes a 32-bit word to the EEPROM emulation.
 * @param  address: The address to write to.
 * @param  data: The data to write.
 * @retval HAL status.
 */
HAL_StatusTypeDef eeprom_write(uint32_t address, uint32_t data)
{
    if (address >= EEPROM_SIZE)
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status = HAL_FLASH_Unlock();

    if (status == HAL_OK)
    {
        // Erase the sector before writing
        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t SectorError = 0;

        EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        EraseInitStruct.Sector = FLASH_SECTOR_4; // Assuming the last sector is used for EEPROM
        EraseInitStruct.NbSectors = 1;

        if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
        {
            return HAL_ERROR;
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&_eeprom_start + address, data);

        HAL_FLASH_Lock();
    }

    return status;
}

#ifndef __EEPROM_H
#define __EEPROM_H

#include "stm32f4xx_hal.h"

// Define the size of the EEPROM emulation
#define EEPROM_SIZE 0x4000

// Define the start address of the EEPROM emulation
// This address must be defined in the linker script
extern uint32_t _eeprom_start[];

// Function prototypes
HAL_StatusTypeDef eeprom_read(uint32_t address, uint32_t *data);
__attribute__((section(".RamFunc"))) HAL_StatusTypeDef eeprom_write(uint32_t address, uint32_t data);

#define EEPROM_MODE_CONFIG 0x00

typedef enum {
    AMIGA_MODE = 0x00,
    PC_MODE = 0x01
} EepromMode;

extern volatile EepromMode current_mode;

#endif /* __EEPROM_H */

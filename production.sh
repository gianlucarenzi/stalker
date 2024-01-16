#!/bin/bash

# The bootloader can be in elf, or hex file
BOOTLOADER=stalkb-bootloader/build/stalkb-bootloader.elf
# The application MUST be hex file
APPLICATION=build/stm32f401-usb-keyboard-amiga.hex

function check_integrity()
{
	local m_EXEC=$1
	if [ ! -f ${m_EXEC} ]; then
		echo "Build $1 first. Please run 'make clean' & 'make' !"
		return 1
	fi
	return 0
}

reset
clear
check_integrity ${BOOTLOADER}
if [ $? -ne 0 ]; then
	exit 1
fi

check_integrity ${APPLICATION}
if [ $? -ne 0 ]; then
	exit 1
fi

# Run bootloader first
./bootloader.sh

# Flash application 
./flash.sh

exit 0

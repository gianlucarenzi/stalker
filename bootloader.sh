#!/bin/bash
EXEC=stalkb-bootloader/build/stalkb-bootloader.elf
if [ ! -f ${EXEC} ]; then
	echo "Build bootloader first. Please run 'make clean' & 'make' !"
	exit 1
fi

# Erase all flash first
openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; flash erase_sector 0 0 last; reset run; shutdown"

openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; flash write_image erase unlock ${EXEC}; reset run; shutdown"

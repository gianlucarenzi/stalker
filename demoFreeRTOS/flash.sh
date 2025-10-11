#!/bin/bash
EXEC=build/demoFreeRTOS.elf
if [ ! -f ${EXEC} ]; then
	echo "Build Application first. Please run 'make clean' & 'make' !"
	exit 1
fi
openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; flash write_image erase unlock ${EXEC}; reset run; shutdown"

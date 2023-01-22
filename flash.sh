#!/bin/bash
EXEC=$1
if [ $# -ne 1 ]; then
	EXEC=build/stm32f401-usb-keyboard-amiga.elf
fi
openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; flash write_image erase unlock ${EXEC}; reset run; shutdown"

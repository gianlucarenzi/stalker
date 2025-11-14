#!/bin/bash
EXEC=$1

if [ "${EXEC}" == "" ]; then
	EXEC=build/stm32f401-usb-keyboard-amiga.hex
fi
if [ ! -f ${EXEC} ]; then
	echo "Build Application first. Please run 'make clean' & 'make' !"
	exit 1
fi
openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; program ${EXEC} verify reset; shutdown"

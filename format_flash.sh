#!/bin/bash
openocd -f openocd/stm32f4eval.cfg \
-c "init; targets; reset init; wait_halt; poll; stm32f4x mass_erase 0; reset run; shutdown"

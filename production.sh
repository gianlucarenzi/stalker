#!/bin/bash

PRJROOT=$(pwd)

# The bootloader can be in elf, or hex file
BOOTLOADER_PATH=${PRJROOT}/stalkb-bootloader
BOOTLOADER=${BOOTLOADER_PATH}/build/stalkb-bootloader.elf

# The application MUST be hex file
APPLICATION_PATH=${PRJROOT}
APPLICATION=${APPLICATION_PATH}/build/stm32f401-usb-keyboard-amiga.hex

# Pass BUILD for build everything
m_FORCE_BUILD=${1}

function check_integrity()
{
	local m_EXEC=$1
	if [ ! -f ${m_EXEC} ]; then
		echo "${m_EXEC} not found"
		return 0
	fi
	return 1
}

reset
clear

if [ "${m_FORCE_BUILD}" != "" ]; then
	echo "BUILD FROM SOURCE..."
	sleep 2
else
	echo "..."
fi

###################### CHECK BOOTLOADER AND BUILD IF NEEDED
check_integrity ${BOOTLOADER}
RES=$?

# Build if forced, or if the artifact doesn't exist (and 'build' is passed)
if [ "${m_FORCE_BUILD}" == "build" ] || [ ${RES} -eq 0 ]; then
	if [ ${RES} -eq 0 ] && [ "${m_FORCE_BUILD}" != "build" ]; then
		echo "Error on checking ${BOOTLOADER}"
		exit 1
	fi

	echo "Building ${BOOTLOADER}..."
	sleep 2
	cd ${BOOTLOADER_PATH}
	make clean
	make -j$(nproc)
	if [ $? -ne 0 ]; then
		echo "Error on building ${BOOTLOADER}"
		exit 1
	fi
	echo "BOOTLOADER Done."
fi

# Final check to ensure the file exists
check_integrity ${BOOTLOADER}
if [ $? -eq 0 ]; then
	echo "Error: ${BOOTLOADER} not found after build attempt."
	exit 1
fi

cd ${PRJROOT}

############################### CHECK APPLICATION AND BUILD IF NEEDED
check_integrity ${APPLICATION}
RES=$?

# Build if forced, or if the artifact doesn't exist (and 'build' is passed)
if [ "${m_FORCE_BUILD}" == "build" ] || [ ${RES} -eq 0 ]; then
	if [ ${RES} -eq 0 ] && [ "${m_FORCE_BUILD}" != "build" ]; then
		echo "Error on checking ${APPLICATION}"
		exit 1
	fi

	echo "Building ${APPLICATION}..."
	sleep 2
	cd ${APPLICATION_PATH}
	make clean
	make -j$(nproc)
	if [ $? -ne 0 ]; then
		echo "Error on building ${APPLICATION}"
		exit 1
	fi
	echo "APPLICATION Done."
fi

# Final check to ensure the file exists
check_integrity ${APPLICATION}
if [ $? -eq 0 ]; then
	echo "Error: ${APPLICATION} not found after build attempt."
	exit 1
fi

cd ${PRJROOT}
echo "Everything is fine..."
# Erase all flash first
echo
echo "FORMAT FLASH"
./format_flash.sh 1>/dev/null 2>/dev/null
echo

# Then flash bootloader first
echo
echo "INSTALLING BOOTLOADER"
./bootloader.sh 1>/dev/null 2>/dev/null

# And finally flash the application
echo
echo "INSTALLING APPLICATION"
./flash.sh 1>/dev/null 2>/dev/null

exit 0

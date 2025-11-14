# STM32 USB Bootloader

This project implements a simple USB bootloader for STM32F401xC microcontrollers. It allows for in-application firmware updates via USB.

## Features

*   **Custom Bootloader:** Provides a mechanism to jump to either the DFU (Device Firmware Upgrade) bootloader or a user application.
*   **Boot Mode Selection:** Determines whether to enter DFU mode or run the application based on a GPIO pin state.
*   **Application Validation:** Checks the validity of the application firmware before jumping to it.
*   **UART Output:** Provides informative messages via UART for debugging and status updates.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

*   **ARM GCC Compiler:** You will need an ARM-none-eabi toolchain.
    *   For Debian/Ubuntu: `sudo apt-get install gcc-arm-none-eabi`
*   **Make:** Build automation tool.
    *   For Debian/Ubuntu: `sudo apt-get install make`
*   **STM32CubeIDE or equivalent:** For project configuration and initial code generation (though this project is already configured).

### Building the Project

To build the project, navigate to the root directory of the repository and run `make`:

```bash
make
```

This will generate the `.elf`, `.hex`, and `.bin` files in the `build/` directory.

### Flashing the Bootloader

The generated `.bin` or `.hex` file can be flashed to your STM32F401xC microcontroller using an ST-Link debugger or similar programming tool.

### Usage

1.  **Power on** your STM32F401xC board.
2.  The bootloader will check the state of the `BOOT_MODE_Pin` (configured in `main.c` and `MX_GPIO_Init`).
    *   If `BOOT_MODE_Pin` is **LOW**, the bootloader will jump to the **DFU Bootloader**.
    *   If `BOOT_MODE_Pin` is **HIGH**, the bootloader will attempt to jump to the **user application**.
3.  Ensure your user application is flashed at the correct address (defined by `__appflash_start` in the linker script).

## Project Structure

*   `Core/`: Contains application-specific source code and headers (`main.c`, `stm32f4xx_it.c`, etc.).
*   `Drivers/`: Contains STM32 HAL drivers and CMSIS files.
*   `STM32F401XX_FLASH.ld`: Linker script for the STM32F401xC.
*   `Makefile`: Build script for the project.
*   `startup_stm32f401xc.s`: Startup assembly file.

## Contributing

Feel free to fork the repository, open issues, or submit pull requests.

## License

This project is licensed under the terms found in the `LICENSE` file in the root directory of this software component.

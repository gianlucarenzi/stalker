# StalkerNG - Amiga Keyboard Interface Firmware

## Project Overview

This project provides firmware for an STM32F401xC microcontroller, designed to bridge modern USB keyboards with classic Amiga computers. It acts as an intelligent interface, translating standard USB Human Interface Device (HID) keyboard inputs into the specific Amiga keyboard protocol. This enables Amiga users to utilize a wide range of contemporary USB keyboards with their vintage systems, enhancing usability without compromising the authentic Amiga experience.

The firmware incorporates advanced features such as dynamic mode switching between Amiga and PC keyboard layouts, comprehensive LED status management (Caps Lock, Num Lock, Scroll Lock), and robust system reset functionalities triggered by specific key combinations or Amiga hardware signals. Built upon the FreeRTOS real-time operating system, it ensures efficient and concurrent management of all tasks, from USB communication to Amiga protocol handling and persistent settings storage.

## Features

*   **Firmware Version:** `v3.0NG-RTOS`
*   **USB HID Keyboard Host:** Full support for connecting and interpreting inputs from standard USB HID keyboards.
*   **Amiga Keyboard Protocol Translation:** Seamless conversion of USB keyboard events into the native Amiga keyboard protocol.
*   **Dynamic Mode Switching:** Toggle between Amiga and PC keyboard layouts using a dedicated key combination (`Left Control + Left Alt + Left Shift + P`). The selected mode is persistently stored in EEPROM.
*   **LED Status Synchronization:** Synchronizes Caps Lock, Num Lock, and Scroll Lock LED states between the USB keyboard and the Amiga system.
*   **System Reset Functionality:**
    *   **PC Mode Reset:** Triggered by holding `Control + Alt + Delete` for 500ms.
    *   **Amiga Mode Reset:** Triggered by holding `Left Control + Left GUI + (Right GUI or Application Key)` for 500ms.
    *   Supports hardware-initiated resets from the Amiga side.
*   **Persistent Settings:** Utilizes internal EEPROM (or Flash emulation) for storing user preferences, such as the last selected operating mode.
*   **Real-Time Operating System (RTOS):** Leverages FreeRTOS for efficient multi-tasking, ensuring responsive and reliable operation.
*   **Easter Egg:** If no keyboard is connected, the firmware will periodically type out a message on the Amiga. This feature is enabled by a compile-time flag.

## Hardware Requirements

*   **Microcontroller:** STM32F401xC (or any compatible STM32F4 series board with USB Host capabilities).
*   **USB Host Port:** Required for connecting USB keyboards.
*   **Amiga Interface Circuitry:** Custom hardware interface to connect the microcontroller to the Amiga keyboard port.

## Software Architecture

The firmware is structured around the STM32Cube HAL and FreeRTOS, providing a robust and modular foundation. A multi-tasking paradigm is employed to manage various functionalities concurrently, ensuring high responsiveness and system stability.

*   **FreeRTOS:** The core of the concurrent execution, managing task scheduling, inter-task communication (via message queues), and precise timing.
*   **STM32 HAL Drivers:** Provides low-level access and control over the STM32 microcontroller's peripherals.
*   **USB Host Library:** Handles the complexities of USB communication, including device enumeration, configuration, and parsing of HID reports from connected keyboards.
*   **Amiga Keyboard Protocol Driver:** Implements the specific timing and data exchange required for communication with the Amiga computer, translating generic USB HID scancodes into Amiga-specific scancodes.
*   **EEPROM Driver:** Manages read and write operations to non-volatile memory, ensuring that user settings and configurations persist across power cycles.
*   **Inter-Task Communication:** Tasks communicate using thread-safe FreeRTOS queues:
    *   `keyboardQueueHandle`: Passes raw keyboard data from `usbTask` to `amigaTask`.
    *   `ledQueueHandle`: Passes LED status changes (Caps/Num/Scroll lock) from `amigaTask` to `usbTask`.
    *   `ledManagerQueueHandle`: Sends commands from `usbTask` to `led_manager_task` to control the onboard status LED.
    *   `amigaTaskQueueHandle`: Used for sending internal commands (like reset notifications) to the `amigaTask` itself.

## Key Components (FreeRTOS Tasks)

The firmware is organized into several FreeRTOS tasks, each responsible for a specific aspect of the system:

### `amigaTask`

This task is the central hub for Amiga-related keyboard logic. Its priority is `osPriorityLow`.

*   **Initialization:** Sets up the Amiga keyboard interface and reads the initial operating mode from EEPROM.
*   **Mode Management:** Determines and sets the `current_mode` (AMIGA_MODE or PC_MODE) based on saved settings or defaults.
*   **Event Processing:** Receives `keyboard_message_t` events from the `usbTask` via `keyboardQueueHandle`.
*   **Protocol Translation:** Converts received USB HID data into the Amiga keyboard protocol and sends it to the Amiga via `amikb_process()`.
*   **Mode Switching:** Detects and acts upon the `Left Control + Left Alt + Left Shift + P` key combination to toggle operating modes.
*   **System Reset Handling:** Monitors for specific key combinations (`Control + Alt + Delete` in PC mode, `Left Control + Left GUI + (Right GUI or Application Key)` in Amiga mode) and Amiga hardware reset signals, initiating a system reset if conditions are met.
*   **LED Feedback:** Sends `led_status_t` updates to the `ledQueueHandle` for visual feedback.

#### `amigaTask` Main Loop Flowchart

```mermaid
graph TD
    A[Start amigaTask] --> B{Initialize Amiga Interface};
    B --> C{Read Mode from EEPROM};
    C --> D{Set current_mode};
    D --> E[amiga_task_init()];
    E --> F[Loop Forever];
    F --> G{osMessageQueueGet(keyboardQueueHandle, msg, 0)};
    G -- osOK --> H{process_keyboard_message(msg)};
    H --> I{convert_message_to_keyboard_code(msg, code)};
    I --> J{amikb_process(code)};
    J --> K{Check if Amiga Interface is ready};
    K -- Not Ready --> L[Set amiga_ready = 1; amikb_ready(1)];
    L --> M{osMessageQueueGet(amigaTaskQueueHandle, internal_msg, 0)};
    G -- Not osOK --> M;
    M -- osOK (RESET_START) --> N[Send LED_RESET_BLINK to ledQueue];
    N --> O[check_for_special_combos()];
    M -- Not osOK --> O;
    O --> P[amiga_task_check_reset_condition()];
    P --> Q[osDelay(10ms)];
    Q --> F;
```

**Timing Considerations for `amigaTask`:**
*   `osMessageQueueGet(keyboardQueueHandle, ..., 0)`: This is a non-blocking call, ensuring the task doesn't halt waiting for keyboard input. Keyboard events are processed as soon as they are available.
*   `osMessageQueueGet(amigaTaskQueueHandle, ..., 0)`: Also non-blocking, for internal task communication.
*   `osDelay(10ms)`: A small delay introduced at the end of the loop to yield CPU time to other tasks and prevent busy-waiting. This results in a cycle time of approximately 10ms.
*   `check_for_special_combos()` and `amiga_task_check_reset_condition()`: Both functions incorporate internal timers (e.g., `RESET_TIMEOUT_MS` of 500ms) to detect sustained key presses or Amiga clock line states, preventing spurious triggers.

### `usbTask`

Responsible for all USB Host related operations, primarily interacting with USB keyboards. Its priority is `osPriorityNormal`.

*   **USB Host Management:** Initializes and manages the USB Host stack.
*   **Device Detection:** Monitors for the connection and disconnection of USB keyboards.
*   **HID Report Processing:** Reads raw HID reports from connected keyboards, sending an update only when the state changes.
*   **Event Generation:** Parses HID data into a structured `keyboard_message_t` format.
*   **Inter-Task Communication:** Sends processed keyboard messages to the `amigaTask` via `keyboardQueueHandle`.
*   **LED Control:** Receives `led_status_t` commands from the `amigaTask` and controls the LEDs on the connected USB keyboard (e.g., Caps Lock, Num Lock).

#### `usbTask` Main Loop Flowchart (Conceptual)

```mermaid
graph TD
    A[Start usbTask] --> B{Initialize USB Host};
    B --> C[Loop Forever];
    C --> D{USBH_Process()};
    D --> E{Check for USB Keyboard State Changes};
    E -- Connected --> F{Read HID Report};
    F --> G{Parse HID Data & Check for Changes};
    G -- State Changed --> H{Create keyboard_message_t};
    H --> I{osMessageQueuePut(keyboardQueueHandle, msg, 0)};
    I --> J{Check for LED Status Messages};
    J -- osOK --> K{Control USB Keyboard LEDs};
    K --> C;
    J -- Not osOK --> C;
    G -- No Change --> J;
    E -- Disconnected --> L[Send Reset Event to amigaTask];
    L --> C;
```

**Timing Considerations for `usbTask`:**
*   `USBH_Process()`: This function is the core of the USB host stack and is called in a loop.
*   `osDelay(10ms)`: The task loop has a 10ms delay, making it responsive to USB events and LED control messages.
*   HID Report Polling: The frequency of reading HID reports is often dictated by the USB HID descriptor of the connected keyboard (e.g., 8ms, 10ms). The `usbTask`'s loop is fast enough to accommodate this polling rate.
*   `osMessageQueuePut(keyboardQueueHandle, msg, 0, 0)`: A non-blocking call to send keyboard events, ensuring the `usbTask` doesn't block if the `amigaTask` queue is temporarily full.

### `eeprom_task`

Manages non-volatile storage operations.

*   **Persistence:** Handles asynchronous read and write requests for system settings (e.g., `current_mode`) to the internal EEPROM (emulated in Flash).
*   **Reliability:** Ensures data integrity during storage operations.

### `led_manager_task`

Controls the onboard status LED for visual feedback. Its priority is `osPriorityLow`.

*   **LED Control:** Receives commands via `ledManagerQueueHandle` from other tasks (like `usbTask`) and manages the state of the single onboard LED to reflect system status (e.g., solid ON for ready, blinking for waiting).

### `log_task`

Handles system logging and debugging output.

*   **Debug Output:** Processes and outputs debug messages generated by other tasks, typically via a UART interface to a connected terminal or debug probe. It is initialized at startup but does not run as a separate task.

## Build Instructions

The project uses a standard GNU Make-based build system.

1.  **Toolchain:** Ensure you have the `arm-none-eabi-gcc` toolchain installed and configured in your system's PATH.
2.  **Navigate:** Open a terminal and navigate to the root directory of this project.
3.  **Clean Build:** To remove all previously compiled files and artifacts, execute:
    ```bash
    make clean
    ```
4.  **Build Firmware:** To compile the source code and link the final firmware image, execute:
    ```bash
    make
    ```
5.  **Output:** Upon successful compilation, the following files will be generated in the `build/` directory:
    *   `StalkerNG.elf`: Executable and Linkable Format (ELF) file, containing debugging information.
    *   `StalkerNG.hex`: Intel HEX format file, commonly used for flashing.
    *   `StalkerNG.bin`: Raw binary image, also suitable for flashing.

## Usage and Operation

1.  **Flash Firmware:** Program the `StalkerNG.bin` file onto your STM32F401xC development board using your preferred flashing tool (e.g., STM32CubeProgrammer, OpenOCD).
2.  **Connect USB Keyboard:** Plug your USB keyboard into the USB Host port on the STM32 board.
3.  **Connect to Amiga:** Connect the custom Amiga interface from your STM32 board to the keyboard port of your Amiga computer.
4.  **Power On:** Power on your STM32 board and Amiga.

### Mode Switching

The firmware supports two operating modes: Amiga Mode and PC Mode.

*   **Toggle Mode:** Press and hold the key combination `Left Control + Left Alt + Left Shift + P`. The mode will toggle between AMIGA_MODE and PC_MODE.
*   **Persistence:** The selected mode is automatically saved to the internal EEPROM and will be restored on subsequent power-ups.

### System Reset

The firmware provides multiple ways to trigger a system reset for the Amiga.

*   **PC Mode Reset:** While in PC Mode, press and hold `Control + Alt + Delete` for approximately 500 milliseconds.
*   **Amiga Mode Reset:** While in Amiga Mode, press and hold `Left Control + Left GUI + (Right GUI or Application Key)` for approximately 500 milliseconds.
*   **Amiga Hardware Reset:** The firmware also detects and responds to a hardware-initiated reset signal from the Amiga computer itself.

### USB Keyboard Compatibility

It is crucial to understand that **not all USB keyboards are compatible** with this firmware. The compatibility depends on how the keyboard communicates its key presses to the host.

This firmware, due to limitations in the underlying STM32 USB Host HID drivers, primarily supports keyboards that operate using the **USB HID Boot Protocol**. This protocol is a simplified subset of the full HID specification, designed to provide basic keyboard (and mouse) functionality even before a full operating system driver is loaded.

Keyboards that rely on the more complex **USB HID Report Protocol** for their functionality (especially those with advanced features, custom layouts, or extensive multimedia keys) are generally **not supported**. The firmware's current HID implementation is not adequate to fully interpret these custom report descriptors.

**What to look for in a compatible keyboard:**

*   **Basic Functionality:** Keyboards designed for general office use or older systems are more likely to implement the Boot Protocol.
*   **Simplicity:** Keyboards without extensive programmable keys, RGB lighting, or complex multimedia functions beyond standard volume/play controls might be more compatible.
*   **Testing is Key:** The best way to determine compatibility is often to test the keyboard with the firmware.

**Known Compatible Example:**

*   **Logitech Wireless Keyboards with Unifying Receiver (e.g., VID 0x046d, PID 0xc534):** Many Logitech wireless keyboard/mouse combinations that use the Unifying Receiver dongle operate in USB BOOT mode for basic functionality.
    *   **Note:** While the basic alphanumeric and modifier keys typically work, **multimedia keys** (e.g., volume, play/pause) on such keyboards are often implemented using the HID Report Protocol and may **not be recognized** by this firmware.

We recommend users to prioritize keyboards known for their basic, robust USB HID compliance. If a keyboard does not work, it is likely due to its reliance on the USB HID Report Protocol.

## Contributing

Contributions are welcome! If you find a bug or have an enhancement in mind, please open an issue or submit a pull request.

## License

This project is licensed under the terms found in the `LICENSE` file in the root directory of this repository.

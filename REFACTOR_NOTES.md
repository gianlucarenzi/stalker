# FreeRTOS Refactor - USB to Amiga Keyboard Adapter

## Overview

This document describes the refactoring of the USB to Amiga keyboard adapter project to use FreeRTOS with two separate tasks for better separation of concerns and improved real-time performance.

## Architecture Changes

### Before Refactor
- Single main loop handling both USB and Amiga communication
- Blocking operations and polling-based approach
- Tight coupling between USB and Amiga logic

### After Refactor
- Two separate FreeRTOS tasks:
  - **usb_task**: Handles USB HID protocol and LED management
  - **amiga_task**: Handles GPIO communication with Amiga and scancode processing
- Queue-based communication between tasks
- Non-blocking, event-driven architecture

## Task Structure

### USB Task (`usb_task`)
**Priority**: 3 (High)
**Stack Size**: 512 words (configMINIMAL_STACK_SIZE * 4)
**Responsibilities**:
- Initialize and manage USB Host
- Process USB HID events
- Handle keyboard connection/disconnection
- Read keyboard data from USB
- Manage USB keyboard LEDs
- Send keyboard data to Amiga task via queue
- Receive LED commands from Amiga task via queue

**Key Functions**:
- `usb_task()` - Main task function
- `usb_task_init()` - Task initialization
- `usb_keyboard_led_set()` - Set keyboard LED state
- `usb_keyboard_led_init_sequence()` - LED initialization sequence

### Amiga Task (`amiga_task`)
**Priority**: 2 (Medium)
**Stack Size**: 256 words (configMINIMAL_STACK_SIZE * 2)
**Responsibilities**:
- Initialize Amiga GPIO interface
- Process keyboard data from USB task
- Convert USB scancodes to Amiga scancodes
- Handle Amiga keyboard protocol timing
- Manage Amiga reset conditions
- Send LED status updates to USB task

**Key Functions**:
- `amiga_task()` - Main task function
- `amiga_task_init()` - Task initialization
- `amiga_task_handle_reset()` - Handle Amiga reset sequence

## Communication

### Queues
Two FreeRTOS queues handle inter-task communication:

1. **keyboard_queue** (USB → Amiga)
   - Size: 10 messages
   - Type: `keyboard_message_t`
   - Contains: keyboard data + timestamp

2. **led_queue** (Amiga → USB)
   - Size: 5 messages
   - Type: `led_message_t`
   - Contains: LED status + timestamp

### Message Structures
```c
typedef struct {
    keyboard_code_t keycode;
    uint32_t timestamp;
} keyboard_message_t;

typedef struct {
    led_status_t led_status;
    uint32_t timestamp;
} led_message_t;
```

## Files Added

### Header Files
- `inc/task_communication.h` - Common definitions for task communication
- `inc/usb_task.h` - USB task interface
- `inc/amiga_task.h` - Amiga task interface

### Source Files
- `src/usb_task.c` - USB task implementation
- `src/amiga_task.c` - Amiga task implementation

### Modified Files
- `src/main.c` - Refactored to create tasks and start scheduler
- `Makefile` - Added new source files

## Benefits of Refactor

1. **Separation of Concerns**: USB and Amiga logic are now completely separate
2. **Better Real-time Performance**: High-priority USB task ensures responsive USB handling
3. **Improved Maintainability**: Each task has a clear, focused responsibility
4. **Scalability**: Easy to add new features or modify individual components
5. **Debugging**: Easier to debug issues in specific subsystems
6. **Resource Management**: Better control over stack usage and CPU time

## Timing Considerations

- USB task runs at 10ms intervals for responsive USB handling
- Amiga task runs at 50ms intervals for reset condition checking
- Critical Amiga timing (20μs, 85μs) still uses hardware delays (udelay)
- FreeRTOS tick rate: 1ms (1000 Hz)

## Memory Usage

- **Total heap size**: 17KB (configTOTAL_HEAP_SIZE)
- **USB task stack**: ~2KB
- **Amiga task stack**: ~1KB
- **Queue memory**: ~500 bytes total
- **Remaining heap**: ~13.5KB for other allocations

## Configuration

Key FreeRTOS configuration parameters in `FreeRTOSConfig.h`:
- `configTICK_RATE_HZ`: 1000 (1ms tick)
- `configMAX_PRIORITIES`: 5
- `configTOTAL_HEAP_SIZE`: 17KB
- `configUSE_PREEMPTION`: 1 (preemptive scheduling)

## Future Enhancements

Possible future improvements:
1. Add watchdog task for system monitoring
2. Implement power management features
3. Add configuration task for runtime settings
4. Implement USB device detection improvements
5. Add statistics collection task

## Testing Notes

When testing the refactored code:
1. Verify USB keyboard detection and LED functionality
2. Test Amiga reset sequence
3. Check for proper task scheduling and no deadlocks
4. Verify timing-critical Amiga protocol operations
5. Test under various load conditions

## Compatibility

The refactored code maintains full compatibility with:
- Original hardware design
- Amiga keyboard protocol
- USB HID keyboard standard
- All existing features and Easter eggs
#ifndef __SYSCALL_INCLUDED__
#define __SYSCALL_INCLUDED__

#include <stdio.h>
#include <stm32f4xx_hal.h>
typedef enum {
	SYSCALL_NOTREADY,
	SYSCALL_READY,
} t_syscall_status;

extern void _write_ready(t_syscall_status rdy, UART_HandleTypeDef *ptr);
extern int _write(int file, char *data, int len);

/**
 * Simple timer functions!
 **/
extern void timer_start(void);
extern int timer_elapsed(uint32_t msec);

/**
 * blocking functions!
 **/
extern void mdelay(uint32_t millis);
extern void udelay(uint32_t micros);

#endif

#ifndef UART_PRINT_H
#define UART_PRINT_H
#include <stdint.h>
void UART_SendByte(uint8_t b);
void UART_SendString(const char *s);
void UART_Printf(const char *fmt, ...);
#endif

#include "ti_msp_dl_config.h"
#include "uart_print.h"
#include <stdio.h>
#include <stdarg.h>

void UART_SendByte(uint8_t b)
{
    while (DL_UART_isBusy(UART_0_INST));
    DL_UART_Main_transmitData(UART_0_INST, b);
}

void UART_SendString(const char *s)
{
    while (*s) UART_SendByte((uint8_t)*s++);
}

void UART_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    UART_SendString(buf);
}

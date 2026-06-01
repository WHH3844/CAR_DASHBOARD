#ifndef UART_H
#define UART_H

#include <stdint.h>

void Uart_DebugInit(void);
void Uart_DebugPutc(char ch);
void Uart_DebugPuts(const char *str);
void Uart_DebugPutHex32(uint32_t value);
void Uart_DebugPutDec(uint32_t value);

#endif /* UART_H */

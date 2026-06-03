#ifndef UARTIF_H
#define UARTIF_H

#include <stdint.h>

void UartIf_Init(void);
void UartIf_WriteString(const char *text);
void UartIf_WriteHex32(uint32_t value);
void UartIf_WriteDec(uint32_t value);

#endif /* UARTIF_H */

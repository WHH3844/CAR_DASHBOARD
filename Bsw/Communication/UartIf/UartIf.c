#include "UartIf.h"

#include "Uart.h"

void UartIf_Init(void)
{
    Uart_DebugInit();
}

void UartIf_WriteString(const char *text)
{
    Uart_DebugPuts(text);
}

void UartIf_WriteHex32(uint32_t value)
{
    Uart_DebugPutHex32(value);
}

void UartIf_WriteDec(uint32_t value)
{
    Uart_DebugPutDec(value);
}

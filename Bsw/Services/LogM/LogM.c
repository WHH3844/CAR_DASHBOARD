#include "LogM.h"

#include "Uart.h"

static uint8_t LogM_Initialized;

void LogM_Init(void)
{
    if (LogM_Initialized == 0u)
    {
        Uart_DebugInit();
        LogM_Initialized = 1u;
    }
}

void LogM_PutString(const char *text)
{
    if (LogM_Initialized == 0u)
    {
        LogM_Init();
    }

    Uart_DebugPuts(text);
}

void LogM_PutHex32(uint32_t value)
{
    if (LogM_Initialized == 0u)
    {
        LogM_Init();
    }

    Uart_DebugPutHex32(value);
}

void LogM_PutDec(uint32_t value)
{
    if (LogM_Initialized == 0u)
    {
        LogM_Init();
    }

    Uart_DebugPutDec(value);
}

void LogM_NewLine(void)
{
    LogM_PutString("\n");
}

void LogM_Info(const char *text)
{
    LogM_PutString("[INFO] ");
    LogM_PutString(text);
    LogM_NewLine();
}

void LogM_Warn(const char *text)
{
    LogM_PutString("[WARN] ");
    LogM_PutString(text);
    LogM_NewLine();
}

void LogM_Error(const char *text)
{
    LogM_PutString("[ERROR] ");
    LogM_PutString(text);
    LogM_NewLine();
}

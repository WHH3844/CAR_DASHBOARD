#include "App_Logger.h"

void App_Logger_Init(void)
{
}

void App_Logger_MainFunction(uint32_t tick_ms)
{
    (void)tick_ms;
}
#include "App_Logger.h"

#include "LogM.h"

static uint16_t App_LoggerTimerMs;

void App_Logger_Init(void)
{
    App_LoggerTimerMs = 0u;
}

void App_Logger_MainFunction(uint16_t elapsed_ms)
{
    App_LoggerTimerMs = (uint16_t)(App_LoggerTimerMs + elapsed_ms);
    if (App_LoggerTimerMs >= 1000u)
    {
        App_LoggerTimerMs = 0u;
        LogM_Info("runtime heartbeat");
    }
}

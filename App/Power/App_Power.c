#include "App_Power.h"

#include "PowerIf.h"
#include "board_pins.h"

static uint8_t App_Power_ReleaseSeen;
static uint8_t App_Power_ShutdownRequested;
static uint32_t App_Power_PressedStartMs;

void App_Power_Init(void)
{
    App_Power_ReleaseSeen = 0u;
    App_Power_ShutdownRequested = 0u;
    App_Power_PressedStartMs = 0u;
}

void App_Power_MainFunction(uint32_t tick_ms)
{
    if (PowerIf_KeyIsPressed() == 0u)
    {
        App_Power_ReleaseSeen = 1u;
        App_Power_PressedStartMs = 0u;
        return;
    }

    /*
     * 刚上电时电源键可能还按着。必须等用户松开过一次，
     * 后续再次长按才认为是关机请求。
     */
    if (App_Power_ReleaseSeen == 0u)
    {
        return;
    }

    if (App_Power_PressedStartMs == 0u)
    {
        App_Power_PressedStartMs = tick_ms;
        return;
    }

    if ((tick_ms - App_Power_PressedStartMs) >= POWERIF_SHUTDOWN_LONG_PRESS_MS)
    {
        App_Power_ShutdownRequested = 1u;
    }
}

uint8_t App_Power_IsShutdownRequested(void)
{
    return App_Power_ShutdownRequested;
}

void App_Power_ClearShutdownRequest(void)
{
    App_Power_ShutdownRequested = 0u;
}
#include "App_Power.h"

#include "NvM.h"
#include "PowerIf.h"
#include "board_pins.h"

static uint8_t App_PowerReleaseSeen;
static uint16_t App_PowerPressedMs;

void App_Power_Init(void)
{
    App_PowerReleaseSeen = 0u;
    App_PowerPressedMs = 0u;
}

void App_Power_MainFunction(uint16_t elapsed_ms)
{
    if (PowerIf_KeyIsPressed() == 0u)
    {
        App_PowerReleaseSeen = 1u;
        App_PowerPressedMs = 0u;
        return;
    }

    if (App_PowerReleaseSeen == 0u)
    {
        return;
    }

    if (App_PowerPressedMs < POWERIF_SHUTDOWN_LONG_PRESS_MS)
    {
        App_PowerPressedMs = (uint16_t)(App_PowerPressedMs + elapsed_ms);
        return;
    }

    /*
     * 软关机前先强制写 NvM，再拉低 PWR_HOLD。
     * 这比直接在按键回调里断电更接近真实 ECU 的关机流程。
     */
    (void)NvM_WriteAll();
    PowerIf_Shutdown();
}

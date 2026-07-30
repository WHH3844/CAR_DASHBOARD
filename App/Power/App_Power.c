#include "App_Power.h"

#include "PowerIf.h"
#include "Rte_Signal.h"
#include "board_pins.h"

/*
 * ReleaseSeen 防止上电按住电源键时直接触发关机。
 * ShutdownRequested 由 App_Power 置位，由 EcuM 读取并清除。
 * PressedStartMs 记录本次长按开始时间，0 表示当前没有计时。
 */
static uint8_t App_Power_ReleaseSeen;
static uint8_t App_Power_ShutdownRequested;
static uint32_t App_Power_PressedStartMs;

void App_Power_Init(void)
{
    App_Power_ReleaseSeen = 0u;
    App_Power_ShutdownRequested = 0u;
    App_Power_PressedStartMs = 0u;
    (void)Rte_Write_ShutdownRequest(0u);
}

void App_Power_MainFunction(uint32_t tick_ms)
{
    if (PowerIf_KeyIsPressed() == 0u)
    {
        /*
         * 检测到释放后，后续再按才进入长按关机判定。
         * 每次释放都清零 PressedStartMs，避免下一次按下沿沿用旧时间。
         */
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
        /* 第一次看到按下时只记录起点，下一轮再计算持续时间。 */
        App_Power_PressedStartMs = tick_ms;
        return;
    }

    if ((tick_ms - App_Power_PressedStartMs) >= POWERIF_SHUTDOWN_LONG_PRESS_MS)
    {
        App_Power_ShutdownRequested = 1u;
        (void)Rte_Write_ShutdownRequest(1u);
    }
}

uint8_t App_Power_IsShutdownRequested(void)
{
    return App_Power_ShutdownRequested;
}

void App_Power_ClearShutdownRequest(void)
{
    App_Power_ShutdownRequested = 0u;
    (void)Rte_Write_ShutdownRequest(0u);
}

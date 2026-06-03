#include "App_Key.h"

#include "IoHwAb.h"
#include "Rte_Event.h"

#define APP_KEY_DEBOUNCE_MS     40u

/*
 * LastRawMask：最近一次读取到的原始电平。
 * StableMask：已经通过去抖确认的稳定电平。
 * LastChangeTick：原始电平最后一次变化时间，用于判断是否稳定超过 40ms。
 */
static uint8_t App_Key_LastRawMask;
static uint8_t App_Key_StableMask;
static uint32_t App_Key_LastChangeTick;

void App_Key_Init(void)
{
    IoHwAb_KeyInit();
    App_Key_LastRawMask = IoHwAb_ReadUserKeyMask();
    App_Key_StableMask = App_Key_LastRawMask;
    App_Key_LastChangeTick = 0u;
}

static void App_Key_PublishPressed(uint8_t key_mask)
{
    if (key_mask == IOHWAB_KEY1_MASK)
    {
        Rte_Event_PublishKey(RTE_KEY_EVENT_KEY1_SHORT);
    }
    else if (key_mask == IOHWAB_KEY2_MASK)
    {
        Rte_Event_PublishKey(RTE_KEY_EVENT_KEY2_SHORT);
    }
    else if (key_mask == IOHWAB_KEY3_MASK)
    {
        Rte_Event_PublishKey(RTE_KEY_EVENT_KEY3_SHORT);
    }
}

void App_Key_MainFunction(uint32_t tick_ms)
{
    uint8_t raw_mask;
    uint8_t changed;

    raw_mask = IoHwAb_ReadUserKeyMask();
    if (raw_mask != App_Key_LastRawMask)
    {
        /*
         * 原始电平刚发生变化时先不发布事件。
         * 只有它持续稳定超过 APP_KEY_DEBOUNCE_MS，才认为是真的按键动作。
         */
        App_Key_LastRawMask = raw_mask;
        App_Key_LastChangeTick = tick_ms;
        return;
    }

    if ((tick_ms - App_Key_LastChangeTick) < APP_KEY_DEBOUNCE_MS)
    {
        return;
    }

    if (raw_mask == App_Key_StableMask)
    {
        return;
    }

    /*
     * 这里只发布“按下沿”，释放沿先不关心。
     * APP_Dashboard 收到事件后决定切模拟、静音或清零。
     * changed 标出本轮稳定状态变化的按键，raw_mask 为 1 表示变化后的状态是按下。
     */
    changed = (uint8_t)(raw_mask ^ App_Key_StableMask);
    if (((changed & IOHWAB_KEY1_MASK) != 0u) && ((raw_mask & IOHWAB_KEY1_MASK) != 0u))
    {
        App_Key_PublishPressed(IOHWAB_KEY1_MASK);
    }
    if (((changed & IOHWAB_KEY2_MASK) != 0u) && ((raw_mask & IOHWAB_KEY2_MASK) != 0u))
    {
        App_Key_PublishPressed(IOHWAB_KEY2_MASK);
    }
    if (((changed & IOHWAB_KEY3_MASK) != 0u) && ((raw_mask & IOHWAB_KEY3_MASK) != 0u))
    {
        App_Key_PublishPressed(IOHWAB_KEY3_MASK);
    }

    App_Key_StableMask = raw_mask;
}

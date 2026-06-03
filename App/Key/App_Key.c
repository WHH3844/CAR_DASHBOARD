#include "App_Key.h"

#include "IoHwAb.h"
#include "Rte_Event.h"

#define APP_KEY_DEBOUNCE_MS     40u

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
#include "App_Key.h"

#include "App_Cfg.h"
#include "IoHwAb.h"
#include "Rte_Event.h"
#include "Rte_Signal.h"

#define APP_KEY_DEBOUNCE_MS    40u

static uint8_t App_KeyRaw;
static uint8_t App_KeyLastRaw;
static uint8_t App_KeyStable;
static uint16_t App_KeyStableAgeMs;

static void App_Key_HandlePressed(uint8_t key_mask)
{
    if (key_mask == IOHWAB_KEY1_MASK)
    {
        Rte_EventPushKey(RTE_KEY_EVENT_KEY1_SHORT);
        (void)Rte_Write_KeyEvent(RTE_KEY_EVENT_KEY1_SHORT);
    }
    else if (key_mask == IOHWAB_KEY2_MASK)
    {
        Rte_EventPushKey(RTE_KEY_EVENT_KEY2_SHORT);
        (void)Rte_Write_KeyEvent(RTE_KEY_EVENT_KEY2_SHORT);
    }
    else if (key_mask == IOHWAB_KEY3_MASK)
    {
        Rte_EventPushKey(RTE_KEY_EVENT_KEY3_SHORT);
        (void)Rte_Write_KeyEvent(RTE_KEY_EVENT_KEY3_SHORT);
    }
}

static void App_Key_HandleChanged(uint8_t old_mask, uint8_t new_mask)
{
    uint8_t changed;

    changed = (uint8_t)(old_mask ^ new_mask);
    if (((changed & IOHWAB_KEY1_MASK) != 0u) && ((new_mask & IOHWAB_KEY1_MASK) != 0u))
    {
        App_Key_HandlePressed(IOHWAB_KEY1_MASK);
    }
    if (((changed & IOHWAB_KEY2_MASK) != 0u) && ((new_mask & IOHWAB_KEY2_MASK) != 0u))
    {
        App_Key_HandlePressed(IOHWAB_KEY2_MASK);
    }
    if (((changed & IOHWAB_KEY3_MASK) != 0u) && ((new_mask & IOHWAB_KEY3_MASK) != 0u))
    {
        App_Key_HandlePressed(IOHWAB_KEY3_MASK);
    }
}

void App_Key_Init(void)
{
    IoHwAb_KeyInit();
    Rte_EventInit();
    App_KeyRaw = IoHwAb_ReadUserKeyMask();
    App_KeyLastRaw = App_KeyRaw;
    App_KeyStable = App_KeyRaw;
    App_KeyStableAgeMs = 0u;
}

void App_Key_MainFunction(uint16_t elapsed_ms)
{
    App_KeyRaw = IoHwAb_ReadUserKeyMask();
    if (App_KeyRaw == App_KeyLastRaw)
    {
        if (App_KeyStableAgeMs < APP_KEY_DEBOUNCE_MS)
        {
            App_KeyStableAgeMs = (uint16_t)(App_KeyStableAgeMs + elapsed_ms);
        }
    }
    else
    {
        App_KeyLastRaw = App_KeyRaw;
        App_KeyStableAgeMs = 0u;
    }

    if ((App_KeyStableAgeMs >= APP_KEY_DEBOUNCE_MS) && (App_KeyStable != App_KeyRaw))
    {
        App_Key_HandleChanged(App_KeyStable, App_KeyRaw);
        App_KeyStable = App_KeyRaw;
    }
}

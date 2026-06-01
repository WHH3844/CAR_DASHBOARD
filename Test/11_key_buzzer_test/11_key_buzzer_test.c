#include "key_buzzer_test.h"

#include "BuzzerIf.h"
#include "IoHwAb.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST11_SAMPLE_MS       20u
#define TEST11_DEBOUNCE_MS     40u

static void Test11_DelayMs(uint32_t ms)
{
    uint32_t i;

    while (ms-- != 0u)
    {
        for (i = 0u; i < 20000u; i++)
        {
            __NOP();
        }
    }
}

static void Test11_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test11_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test11_Beep(uint32_t on_ms, uint32_t off_ms)
{
    BuzzerIf_On();
    Test11_WaitWithPowerCheck(on_ms);
    BuzzerIf_Off();
    Test11_WaitWithPowerCheck(off_ms);
}

static void Test11_BeepCount(uint8_t count)
{
    uint8_t index;

    for (index = 0u; index < count; index++)
    {
        Test11_Beep(80u, 80u);
    }
}

static void Test11_PrintKeyName(uint8_t mask)
{
    if (mask == IOHWAB_KEY1_MASK)
    {
        Uart_DebugPuts("KEY1");
    }
    else if (mask == IOHWAB_KEY2_MASK)
    {
        Uart_DebugPuts("KEY2");
    }
    else if (mask == IOHWAB_KEY3_MASK)
    {
        Uart_DebugPuts("KEY3");
    }
    else
    {
        Uart_DebugPuts("KEY?");
    }
}

static void Test11_PrintState(uint8_t mask)
{
    Uart_DebugPuts("[KEY] state K1=");
    Uart_DebugPutDec((mask & IOHWAB_KEY1_MASK) ? 1u : 0u);
    Uart_DebugPuts(" K2=");
    Uart_DebugPutDec((mask & IOHWAB_KEY2_MASK) ? 1u : 0u);
    Uart_DebugPuts(" K3=");
    Uart_DebugPutDec((mask & IOHWAB_KEY3_MASK) ? 1u : 0u);
    Uart_DebugPuts("\n");
}

static void Test11_PrintKeyEvent(uint8_t key_mask, uint8_t pressed)
{
    Uart_DebugPuts("[KEY] ");
    Test11_PrintKeyName(key_mask);
    if (pressed != 0u)
    {
        Uart_DebugPuts(" pressed\n");
    }
    else
    {
        Uart_DebugPuts(" released\n");
    }
}

static void Test11_HandleKeyPressed(uint8_t key_mask)
{
    if (key_mask == IOHWAB_KEY1_MASK)
    {
        Test11_BeepCount(1u);
    }
    else if (key_mask == IOHWAB_KEY2_MASK)
    {
        Test11_BeepCount(2u);
    }
    else if (key_mask == IOHWAB_KEY3_MASK)
    {
        Test11_BeepCount(3u);
    }
}

static void Test11_HandleChangedKeys(uint8_t old_mask, uint8_t new_mask)
{
    uint8_t changed;
    uint8_t key;

    changed = (uint8_t)(old_mask ^ new_mask);

    key = IOHWAB_KEY1_MASK;
    if ((changed & key) != 0u)
    {
        Test11_PrintKeyEvent(key, (new_mask & key) ? 1u : 0u);
        if ((new_mask & key) != 0u)
        {
            Test11_HandleKeyPressed(key);
        }
    }

    key = IOHWAB_KEY2_MASK;
    if ((changed & key) != 0u)
    {
        Test11_PrintKeyEvent(key, (new_mask & key) ? 1u : 0u);
        if ((new_mask & key) != 0u)
        {
            Test11_HandleKeyPressed(key);
        }
    }

    key = IOHWAB_KEY3_MASK;
    if ((changed & key) != 0u)
    {
        Test11_PrintKeyEvent(key, (new_mask & key) ? 1u : 0u);
        if ((new_mask & key) != 0u)
        {
            Test11_HandleKeyPressed(key);
        }
    }
}

void Test11_KeyBuzzer_Run(void)
{
    uint8_t raw_mask;
    uint8_t last_raw_mask;
    uint8_t stable_mask;
    uint32_t same_ms;
    uint32_t alive_ms;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 11_key_buzzer_test start\n");
    Uart_DebugPuts("[INFO] KEY1=PF6 KEY2=PF7 KEY3=PF8, active low\n");
    Uart_DebugPuts("[INFO] BUZZER_CTRL=PF9, high level on\n");

    IoHwAb_KeyInit();
    BuzzerIf_Init();

    Uart_DebugPuts("[INFO] buzzer startup beep\n");
    Test11_BeepCount(2u);

    raw_mask = IoHwAb_ReadUserKeyMask();
    last_raw_mask = raw_mask;
    stable_mask = raw_mask;
    same_ms = 0u;
    alive_ms = 0u;

    Test11_PrintState(stable_mask);
    Uart_DebugPuts("[PASS] 11_key_buzzer_test running\n");

    while (1)
    {
        raw_mask = IoHwAb_ReadUserKeyMask();
        if (raw_mask == last_raw_mask)
        {
            if (same_ms < TEST11_DEBOUNCE_MS)
            {
                same_ms += TEST11_SAMPLE_MS;
            }
        }
        else
        {
            last_raw_mask = raw_mask;
            same_ms = 0u;
        }

        if ((same_ms >= TEST11_DEBOUNCE_MS) && (stable_mask != raw_mask))
        {
            Test11_HandleChangedKeys(stable_mask, raw_mask);
            stable_mask = raw_mask;
            Test11_PrintState(stable_mask);
        }

        alive_ms += TEST11_SAMPLE_MS;
        if (alive_ms >= 1000u)
        {
            Uart_DebugPuts("[BOOT] key buzzer test alive\n");
            alive_ms = 0u;
        }

        Test11_WaitWithPowerCheck(TEST11_SAMPLE_MS);
    }
}

#include "rtc_test.h"

#include "PowerIf.h"
#include "RtcIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

static void Test08_DelayMs(uint32_t ms)
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

static void Test08_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test08_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test08_PrintTwoDigits(uint8_t value)
{
    Uart_DebugPutc((char)('0' + (value / 10u)));
    Uart_DebugPutc((char)('0' + (value % 10u)));
}

static void Test08_PrintTime(const char *prefix, const RtcIf_TimeType *time)
{
    Uart_DebugPuts(prefix);
    Uart_DebugPutDec(time->year);
    Uart_DebugPutc('-');
    Test08_PrintTwoDigits(time->month);
    Uart_DebugPutc('-');
    Test08_PrintTwoDigits(time->date);
    Uart_DebugPutc(' ');
    Test08_PrintTwoDigits(time->hour);
    Uart_DebugPutc(':');
    Test08_PrintTwoDigits(time->minute);
    Uart_DebugPutc(':');
    Test08_PrintTwoDigits(time->second);
    Uart_DebugPuts(" week=");
    Uart_DebugPutDec(time->weekday);
    Uart_DebugPuts("\n");
}

static uint32_t Test08_TimeToSeconds(const RtcIf_TimeType *time)
{
    return (((uint32_t)time->hour * 3600u) +
            ((uint32_t)time->minute * 60u) +
            (uint32_t)time->second);
}

void Test08_Rtc_Run(void)
{
    const RtcIf_TimeType default_time =
    {
        2026u, 6u, 1u, 1u, 12u, 0u, 0u
    };
    RtcIf_TimeType time_a;
    RtcIf_TimeType time_b;
    uint8_t status;
    uint8_t need_set_time;
    uint8_t second_changed;
    uint8_t retry;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 08_rtc_test start\n");
    Uart_DebugPuts("[INFO] chip=DS3231, addr=0x68\n");

    if (RtcIf_Init() == 0u)
    {
        Uart_DebugPuts("[FAIL] DS3231 not responding\n");
        while (1)
        {
            Test08_WaitWithPowerCheck(500u);
        }
    }
    Uart_DebugPuts("[PASS] DS3231 ack\n");

    if (RtcIf_StartOscillator() == 0u)
    {
        Uart_DebugPuts("[FAIL] start DS3231 oscillator\n");
        while (1)
        {
            Test08_WaitWithPowerCheck(500u);
        }
    }

    status = 0u;
    if (RtcIf_ReadStatus(&status) == 0u)
    {
        Uart_DebugPuts("[FAIL] read DS3231 status\n");
        while (1)
        {
            Test08_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[INFO] status=");
    Uart_DebugPutHex32(status);
    Uart_DebugPuts("\n");

    need_set_time = 0u;
    if ((status & 0x80u) != 0u)
    {
        Uart_DebugPuts("[WARN] oscillator stop flag set, write default test time\n");
        need_set_time = 1u;
    }

    if ((RtcIf_ReadTime(&time_a) == 0u) || (RtcIf_IsTimeValid(&time_a) == 0u))
    {
        Uart_DebugPuts("[WARN] RTC time invalid, write default test time\n");
        need_set_time = 1u;
    }

    if (need_set_time != 0u)
    {
        if (RtcIf_SetTime(&default_time) == 0u)
        {
            Uart_DebugPuts("[FAIL] set DS3231 time\n");
            while (1)
            {
                Test08_WaitWithPowerCheck(500u);
            }
        }

        (void)RtcIf_ClearOscStopFlag();
        Test08_WaitWithPowerCheck(50u);
    }

    if (RtcIf_ReadTime(&time_a) == 0u)
    {
        Uart_DebugPuts("[FAIL] read DS3231 time\n");
        while (1)
        {
            Test08_WaitWithPowerCheck(500u);
        }
    }
    Test08_PrintTime("[TIME] first=", &time_a);

    second_changed = 0u;
    for (retry = 0u; retry < 5u; retry++)
    {
        Test08_WaitWithPowerCheck(1100u);
        if (RtcIf_ReadTime(&time_b) == 0u)
        {
            Uart_DebugPuts("[FAIL] read DS3231 time again\n");
            while (1)
            {
                Test08_WaitWithPowerCheck(500u);
            }
        }

        Test08_PrintTime("[TIME] second=", &time_b);

        if (Test08_TimeToSeconds(&time_a) != Test08_TimeToSeconds(&time_b))
        {
            second_changed = 1u;
            break;
        }
    }

    if (second_changed == 0u)
    {
        Uart_DebugPuts("[FAIL] second not changed\n");
        while (1)
        {
            Test08_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[PASS] 08_rtc_test all passed\n");
    Uart_DebugPuts("[INFO] power off for 1~5 minutes, then run again to verify CR1220 backup\n");

    while (1)
    {
        if (RtcIf_ReadTime(&time_a) != 0u)
        {
            Test08_PrintTime("[TIME] now=", &time_a);
        }
        else
        {
            Uart_DebugPuts("[WARN] read time failed\n");
        }

        Test08_WaitWithPowerCheck(1000u);
    }
}

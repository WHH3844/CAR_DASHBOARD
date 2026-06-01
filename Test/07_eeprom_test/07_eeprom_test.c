#include "eeprom_test.h"

#include "Eep.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST07_BASE_ADDRESS          0x0100u
#define TEST07_TEST_LEN              16u

static void Test07_DelayMs(uint32_t ms)
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

static void Test07_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test07_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test07_PrintHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    Uart_DebugPuts("0x");
    Uart_DebugPutc(hex[(value >> 4u) & 0x0Fu]);
    Uart_DebugPutc(hex[value & 0x0Fu]);
}

static void Test07_PrintFail(uint16_t address, uint8_t expected, uint8_t actual)
{
    Uart_DebugPuts("[FAIL] addr=");
    Uart_DebugPutHex32(address);
    Uart_DebugPuts(" exp=");
    Test07_PrintHex8(expected);
    Uart_DebugPuts(" act=");
    Test07_PrintHex8(actual);
    Uart_DebugPuts("\n");
}

static uint8_t Test07_WritePattern(const uint8_t *pattern)
{
    uint8_t index;

    for (index = 0u; index < TEST07_TEST_LEN; index++)
    {
        if (Eep_WriteByte((uint16_t)(TEST07_BASE_ADDRESS + index), pattern[index]) == 0u)
        {
            Uart_DebugPuts("[FAIL] write addr=");
            Uart_DebugPutHex32(TEST07_BASE_ADDRESS + index);
            Uart_DebugPuts("\n");
            return 0u;
        }

        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
    }

    return 1u;
}

static uint8_t Test07_VerifyPattern(const uint8_t *pattern)
{
    uint8_t index;
    uint8_t actual;

    for (index = 0u; index < TEST07_TEST_LEN; index++)
    {
        actual = 0u;
        if (Eep_ReadByte((uint16_t)(TEST07_BASE_ADDRESS + index), &actual) == 0u)
        {
            Uart_DebugPuts("[FAIL] read addr=");
            Uart_DebugPutHex32(TEST07_BASE_ADDRESS + index);
            Uart_DebugPuts("\n");
            return 0u;
        }

        if (actual != pattern[index])
        {
            Test07_PrintFail((uint16_t)(TEST07_BASE_ADDRESS + index),
                             pattern[index],
                             actual);
            return 0u;
        }

        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
    }

    return 1u;
}

static uint8_t Test07_RunPattern(const char *name, const uint8_t *pattern)
{
    Uart_DebugPuts("[INFO] write ");
    Uart_DebugPuts(name);
    Uart_DebugPuts("\n");

    if (Test07_WritePattern(pattern) == 0u)
    {
        return 0u;
    }

    if (Test07_VerifyPattern(pattern) == 0u)
    {
        return 0u;
    }

    Uart_DebugPuts("[PASS] ");
    Uart_DebugPuts(name);
    Uart_DebugPuts(" readback\n");
    return 1u;
}

void Test07_Eeprom_Run(void)
{
    static const uint8_t pattern_a[TEST07_TEST_LEN] =
    {
        0xA5u, 0x5Au, 0x00u, 0xFFu,
        0x11u, 0x22u, 0x33u, 0x44u,
        0x55u, 0x66u, 0x77u, 0x88u,
        0x99u, 0xAAu, 0xBBu, 0xCCu
    };
    static const uint8_t pattern_b[TEST07_TEST_LEN] =
    {
        0x5Au, 0xA5u, 0xFFu, 0x00u,
        0xEEu, 0xDDu, 0xCCu, 0xBBu,
        0xAAu, 0x99u, 0x88u, 0x77u,
        0x66u, 0x55u, 0x44u, 0x33u
    };

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 07_eeprom_test start\n");
    Uart_DebugPuts("[INFO] chip=FT24C16A, size=");
    Uart_DebugPutDec(EEP_FT24C16A_SIZE_BYTES);
    Uart_DebugPuts(" bytes, test base=");
    Uart_DebugPutHex32(TEST07_BASE_ADDRESS);
    Uart_DebugPuts("\n");

    Eep_Init();

    if (Test07_RunPattern("pattern A", pattern_a) == 0u)
    {
        Uart_DebugPuts("[FAIL] 07_eeprom_test stopped\n");
        while (1)
        {
            Test07_WaitWithPowerCheck(500u);
        }
    }

    if (Test07_RunPattern("pattern B", pattern_b) == 0u)
    {
        Uart_DebugPuts("[FAIL] 07_eeprom_test stopped\n");
        while (1)
        {
            Test07_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[PASS] 07_eeprom_test all passed\n");
    Uart_DebugPuts("[INFO] power cycle and run again to verify nonvolatile storage\n");

    while (1)
    {
        Uart_DebugPuts("[BOOT] EEPROM test alive\n");
        Test07_WaitWithPowerCheck(1000u);
    }
}

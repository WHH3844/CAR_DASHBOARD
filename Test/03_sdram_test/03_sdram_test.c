#include "sdram_test.h"

#include "Exmc.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST03_FIXED_WORDS          1024u
#define TEST03_BLOCK_WORDS          (1024u * 1024u / 4u)
#define TEST03_POWER_POLL_MASK      0x0FFFu
#define TEST03_MAX_ADDRESS_FAILS    8u

typedef struct
{
    const char *name;
    uint32_t offset;
} Test03_ProbePointType;

static void Test03_DelayMs(uint32_t ms)
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

static void Test03_PrintFail(const char *name, uint32_t address, uint32_t expected, uint32_t actual)
{
    Uart_DebugPuts("[FAIL] ");
    Uart_DebugPuts(name);
    Uart_DebugPuts(" addr=");
    Uart_DebugPutHex32(address);
    Uart_DebugPuts(" exp=");
    Uart_DebugPutHex32(expected);
    Uart_DebugPuts(" act=");
    Uart_DebugPutHex32(actual);
    Uart_DebugPuts("\n");
}

static void Test03_PrintFail16(const char *name, uint32_t address, uint16_t expected, uint16_t actual)
{
    Test03_PrintFail(name, address, (uint32_t)expected, (uint32_t)actual);
}

static void Test03_PrintPass(const char *name)
{
    Uart_DebugPuts("[PASS] ");
    Uart_DebugPuts(name);
    Uart_DebugPuts("\n");
}

static uint32_t Test03_Log2(uint32_t value)
{
    uint32_t bit;

    bit = 0u;
    while (value > 1u)
    {
        value >>= 1u;
        bit++;
    }

    return bit;
}

static void Test03_PrintAddressHint(uint32_t byte_offset)
{
    static const char * const address_pins[] =
    {
        "A0/PF0", "A1/PF1", "A2/PF2", "A3/PF3", "A4/PF4",
        "A5/PF5", "A6/PF12", "A7/PF13", "A8/PF14",
        "A9/PF15", "A10/PG0", "A11/PG1", "A12/PG2"
    };
    uint32_t bit;
    uint32_t signal_index;

    bit = Test03_Log2(byte_offset);

    Uart_DebugPuts("[INFO] likely signal=");
    if ((bit >= 1u) && (bit <= 9u))
    {
        signal_index = bit - 1u;
        Uart_DebugPuts("column ");
        Uart_DebugPuts(address_pins[signal_index]);
    }
    else if ((bit >= 10u) && (bit <= 11u))
    {
        Uart_DebugPuts((bit == 10u) ? "BA0/PG4" : "BA1/PG5");
    }
    else if ((bit >= 12u) && (bit <= 24u))
    {
        signal_index = bit - 12u;
        Uart_DebugPuts("row ");
        Uart_DebugPuts(address_pins[signal_index]);
    }
    else
    {
        Uart_DebugPuts("out of 32MB range");
    }
    Uart_DebugPuts("\n");
}

static void Test03_ReportAddressFail(const char *name,
                                     uint32_t byte_offset,
                                     uint16_t expected,
                                     uint16_t actual)
{
    Uart_DebugPuts("[INFO] address offset=");
    Uart_DebugPutHex32(byte_offset);
    Uart_DebugPuts("\n");
    Test03_PrintAddressHint(byte_offset);
    Test03_PrintFail16(name, Exmc_SdramBase() + byte_offset, expected, actual);
}

static void Test03_PollPower(uint32_t index)
{
    if ((index & TEST03_POWER_POLL_MASK) == 0u)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
    }
}

static uint8_t Test03_DataBusTest(volatile uint16_t *sdram)
{
    uint32_t bit;
    uint16_t expected;
    uint16_t actual;

    for (bit = 0u; bit < 16u; bit++)
    {
        expected = (uint16_t)((uint16_t)1u << bit);
        sdram[0] = expected;
        actual = sdram[0];
        if (actual != expected)
        {
            Uart_DebugPuts("[INFO] data bit=DQ");
            Uart_DebugPutDec(bit);
            Uart_DebugPuts("\n");
            Test03_PrintFail16("data bus walking 1", Exmc_SdramBase(), expected, actual);
            return 0u;
        }

        expected = (uint16_t)(~((uint16_t)((uint16_t)1u << bit)));
        sdram[0] = expected;
        actual = sdram[0];
        if (actual != expected)
        {
            Uart_DebugPuts("[INFO] data bit=DQ");
            Uart_DebugPutDec(bit);
            Uart_DebugPuts("\n");
            Test03_PrintFail16("data bus walking 0", Exmc_SdramBase(), expected, actual);
            return 0u;
        }
    }

    Test03_PrintPass("data bus DQ0-DQ15");
    return 1u;
}

static uint8_t Test03_AddressBusTest(volatile uint16_t *sdram)
{
    uint32_t offset;
    uint32_t max_offset;
    uint32_t fail_count;
    uint32_t byte_offset;
    uint16_t actual;
    const uint16_t pattern = 0xAAAAu;
    const uint16_t anti_pattern = 0x5555u;

    max_offset = Exmc_SdramSize() / 2u;
    fail_count = 0u;
    sdram[0] = pattern;

    for (offset = 1u; offset < max_offset; offset <<= 1u)
    {
        sdram[offset] = pattern;
    }

    sdram[0] = anti_pattern;

    for (offset = 1u; offset < max_offset; offset <<= 1u)
    {
        actual = sdram[offset];
        if (actual != pattern)
        {
            byte_offset = offset * 2u;
            Test03_ReportAddressFail("address bus alias", byte_offset, pattern, actual);
            fail_count++;
            if (fail_count >= TEST03_MAX_ADDRESS_FAILS)
            {
                break;
            }
        }
    }

    for (offset = 1u; (offset < max_offset) && (fail_count < TEST03_MAX_ADDRESS_FAILS); offset <<= 1u)
    {
        sdram[0] = pattern;
        sdram[offset] = anti_pattern;
        actual = sdram[0];

        if (actual != pattern)
        {
            byte_offset = offset * 2u;
            Test03_ReportAddressFail("address bus short", byte_offset, pattern, actual);
            fail_count++;
        }

        sdram[offset] = pattern;
    }

    if (fail_count != 0u)
    {
        Uart_DebugPuts("[FAIL] address bus fail count=");
        Uart_DebugPutDec(fail_count);
        Uart_DebugPuts("\n");
        return 0u;
    }

    Test03_PrintPass("address bus");
    return 1u;
}

static uint8_t Test03_FixedPatternTest(volatile uint32_t *sdram)
{
    uint32_t patterns[5];
    uint32_t pattern_index;
    uint32_t index;
    uint32_t value;

    patterns[0] = 0x00000000u;
    patterns[1] = 0xFFFFFFFFu;
    patterns[2] = 0x55AA55AAu;
    patterns[3] = 0xAA55AA55u;
    patterns[4] = 0x12345678u;

    for (pattern_index = 0u; pattern_index < 5u; pattern_index++)
    {
        value = patterns[pattern_index];

        for (index = 0u; index < TEST03_FIXED_WORDS; index++)
        {
            sdram[index] = value ^ index;
        }

        for (index = 0u; index < TEST03_FIXED_WORDS; index++)
        {
            if (sdram[index] != (value ^ index))
            {
                Test03_PrintFail("fixed pattern",
                                 Exmc_SdramBase() + (index * 4u),
                                 value ^ index,
                                 sdram[index]);
                return 0u;
            }
        }
    }

    Test03_PrintPass("fixed pattern");
    return 1u;
}

static uint8_t Test03_WalkingTest(volatile uint32_t *sdram)
{
    uint32_t bit;
    uint32_t value;

    for (bit = 0u; bit < 32u; bit++)
    {
        value = (uint32_t)1u << bit;
        sdram[0] = value;
        if (sdram[0] != value)
        {
            Test03_PrintFail("walking 1", Exmc_SdramBase(), value, sdram[0]);
            return 0u;
        }

        value = ~((uint32_t)1u << bit);
        sdram[0] = value;
        if (sdram[0] != value)
        {
            Test03_PrintFail("walking 0", Exmc_SdramBase(), value, sdram[0]);
            return 0u;
        }
    }

    Test03_PrintPass("walking 1/0");
    return 1u;
}

static uint8_t Test03_BlockFillTest(volatile uint32_t *sdram)
{
    uint32_t index;
    uint32_t expected;

    Uart_DebugPuts("[INFO] block fill words=");
    Uart_DebugPutDec(TEST03_BLOCK_WORDS);
    Uart_DebugPuts("\n");

    for (index = 0u; index < TEST03_BLOCK_WORDS; index++)
    {
        sdram[index] = 0xA5A50000u ^ index;
        Test03_PollPower(index);
    }

    for (index = 0u; index < TEST03_BLOCK_WORDS; index++)
    {
        expected = 0xA5A50000u ^ index;
        if (sdram[index] != expected)
        {
            Test03_PrintFail("block fill",
                             Exmc_SdramBase() + (index * 4u),
                             expected,
                             sdram[index]);
            return 0u;
        }

        Test03_PollPower(index);
    }

    Test03_PrintPass("1MB block fill");
    return 1u;
}

static uint8_t Test03_AddressProbeTest(volatile uint32_t *sdram)
{
    static const Test03_ProbePointType points[] =
    {
        {"base", 0u},
        {"1MB", 1u * 1024u * 1024u},
        {"4MB", 4u * 1024u * 1024u},
        {"8MB", 8u * 1024u * 1024u},
        {"16MB", 16u * 1024u * 1024u},
        {"31MB", 31u * 1024u * 1024u}
    };
    uint32_t index;
    uint32_t word_offset;
    uint32_t expected;

    for (index = 0u; index < (sizeof(points) / sizeof(points[0])); index++)
    {
        word_offset = points[index].offset / 4u;
        sdram[word_offset] = 0x5A5A0000u | index;
    }

    for (index = 0u; index < (sizeof(points) / sizeof(points[0])); index++)
    {
        word_offset = points[index].offset / 4u;
        expected = 0x5A5A0000u | index;

        if (sdram[word_offset] != expected)
        {
            Uart_DebugPuts("[INFO] probe point=");
            Uart_DebugPuts(points[index].name);
            Uart_DebugPuts("\n");
            Test03_PrintFail("address probe",
                             Exmc_SdramBase() + points[index].offset,
                             expected,
                             sdram[word_offset]);
            return 0u;
        }
    }

    Test03_PrintPass("address probe");
    return 1u;
}

void Test03_Sdram_Run(void)
{
    volatile uint16_t *sdram16;
    volatile uint32_t *sdram32;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 03_sdram_test start\n");
    Uart_DebugPuts("[INFO] chip=W9825G6KH-6I, bus=16bit, size=32MB, bank=EXMC SDRAM device0\n");
    Uart_DebugPuts("[INFO] SDCLK=HCLK/3, refresh=502\n");

    if (Exmc_SdramInit() == 0u)
    {
        Uart_DebugPuts("[FAIL] SDRAM init timeout\n");
        while (1)
        {
            (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                                POWERIF_SHUTDOWN_SAMPLE_MS);
            Test03_DelayMs(10u);
        }
    }

    Uart_DebugPuts("[PASS] SDRAM init\n");
    Uart_DebugPuts("[INFO] base=");
    Uart_DebugPutHex32(Exmc_SdramBase());
    Uart_DebugPuts(" size=");
    Uart_DebugPutDec(Exmc_SdramSize() / (1024u * 1024u));
    Uart_DebugPuts("MB\n");

    sdram16 = (volatile uint16_t *)Exmc_SdramBase();
    sdram32 = (volatile uint32_t *)Exmc_SdramBase();

    if ((Test03_DataBusTest(sdram16) != 0u) &&
        (Test03_AddressBusTest(sdram16) != 0u) &&
        (Test03_FixedPatternTest(sdram32) != 0u) &&
        (Test03_WalkingTest(sdram32) != 0u) &&
        (Test03_BlockFillTest(sdram32) != 0u) &&
        (Test03_AddressProbeTest(sdram32) != 0u))
    {
        Uart_DebugPuts("[PASS] 03_sdram_test all passed\n");
    }
    else
    {
        Uart_DebugPuts("[FAIL] 03_sdram_test stopped\n");
    }

    while (1)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Uart_DebugPuts("[BOOT] SDRAM test alive\n");
        Test03_DelayMs(1000u);
    }
}

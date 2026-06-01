#include "i2c_scan_test.h"

#include "I2c.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST06_SCAN_TIMEOUT_LOOP     50000u

static void Test06_DelayMs(uint32_t ms)
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

static void Test06_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test06_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test06_PrintHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    Uart_DebugPuts("0x");
    Uart_DebugPutc(hex[(value >> 4u) & 0x0Fu]);
    Uart_DebugPutc(hex[value & 0x0Fu]);
}

static void Test06_PrintAddressHint(uint8_t address)
{
    if ((address >= 0x50u) && (address <= 0x57u))
    {
        Uart_DebugPuts(" FT24C16A/EEPROM");
    }
    else if (address == 0x68u)
    {
        Uart_DebugPuts(" DS3231/RTC");
    }
    else if ((address == 0x44u) || (address == 0x45u))
    {
        Uart_DebugPuts(" SHT30");
    }
    else
    {
        Uart_DebugPuts(" unknown");
    }
}

static uint8_t Test06_RunOneScan(void)
{
    uint8_t address;
    uint8_t found_count;

    found_count = 0u;
    Uart_DebugPuts("[INFO] bus idle level: SCL=");
    Uart_DebugPuts((I2c0_SclIsHigh() != 0u) ? "H" : "L");
    Uart_DebugPuts(" SDA=");
    Uart_DebugPuts((I2c0_SdaIsHigh() != 0u) ? "H" : "L");
    Uart_DebugPuts("\n");

    if ((I2c0_SclIsHigh() == 0u) || (I2c0_SdaIsHigh() == 0u))
    {
        Uart_DebugPuts("[WARN] I2C line is low, check pull-up, soldering, or device power\n");
    }

    for (address = 0x08u; address <= 0x77u; address++)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);

        if (I2c0_ProbeAddress(address, TEST06_SCAN_TIMEOUT_LOOP) != 0u)
        {
            Uart_DebugPuts("[I2C] found ");
            Test06_PrintHex8(address);
            Test06_PrintAddressHint(address);
            Uart_DebugPuts("\n");
            found_count++;
        }
    }

    Uart_DebugPuts("[INFO] scan done, found=");
    Uart_DebugPutDec(found_count);
    Uart_DebugPuts("\n");

    if (found_count == 0u)
    {
        Uart_DebugPuts("[WARN] no I2C device found\n");
    }

    return found_count;
}

void Test06_I2cScan_Run(void)
{
    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 06_i2c_scan_test start\n");
    Uart_DebugPuts("[INFO] I2C0: PB6=SCL PB7=SDA speed=");
    Uart_DebugPutDec(I2C0_BUS_SPEED);
    Uart_DebugPuts("\n");
    Uart_DebugPuts("[INFO] expect: SHT30=0x44/0x45, FT24C16A=0x50~0x57, DS3231=0x68\n");

    I2c0_Init100K();
    Uart_DebugPuts("[PASS] I2C0 init\n");

    while (1)
    {
        (void)Test06_RunOneScan();
        Test06_WaitWithPowerCheck(3000u);
    }
}

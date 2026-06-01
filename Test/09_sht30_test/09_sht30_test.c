#include "sht30_test.h"

#include "PowerIf.h"
#include "SensorIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

static void Test09_DelayMs(uint32_t ms)
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

static void Test09_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test09_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test09_PrintSignedX100(int32_t value)
{
    uint32_t abs_value;

    if (value < 0)
    {
        Uart_DebugPutc('-');
        abs_value = (uint32_t)(-value);
    }
    else
    {
        abs_value = (uint32_t)value;
    }

    Uart_DebugPutDec(abs_value / 100u);
    Uart_DebugPutc('.');
    if ((abs_value % 100u) < 10u)
    {
        Uart_DebugPutc('0');
    }
    Uart_DebugPutDec(abs_value % 100u);
}

static void Test09_PrintHex8(uint8_t value)
{
    uint8_t nibble;

    Uart_DebugPuts("0x");
    nibble = (uint8_t)((value >> 4u) & 0x0Fu);
    Uart_DebugPutc((char)((nibble < 10u) ? ('0' + nibble) : ('A' + nibble - 10u)));
    nibble = (uint8_t)(value & 0x0Fu);
    Uart_DebugPutc((char)((nibble < 10u) ? ('0' + nibble) : ('A' + nibble - 10u)));
}

static void Test09_PrintSht30Fail(void)
{
    const SensorIf_Sht30DebugType *debug;
    uint8_t index;

    debug = SensorIf_Sht30GetDebug();

    Uart_DebugPuts("[FAIL] SHT30 ");
    switch (SensorIf_Sht30GetLastStatus())
    {
    case SENSORIF_SHT30_STATUS_WRITE_FAIL:
        Uart_DebugPuts("write command");
        break;

    case SENSORIF_SHT30_STATUS_READ_FAIL:
        Uart_DebugPuts("read 6 bytes");
        break;

    case SENSORIF_SHT30_STATUS_TEMP_CRC_FAIL:
        Uart_DebugPuts("temp CRC");
        break;

    case SENSORIF_SHT30_STATUS_HUM_CRC_FAIL:
        Uart_DebugPuts("humidity CRC");
        break;

    case SENSORIF_SHT30_STATUS_PARAM:
        Uart_DebugPuts("param");
        break;

    default:
        Uart_DebugPuts("unknown");
        break;
    }

    Uart_DebugPuts(" raw=");
    for (index = 0u; index < sizeof(debug->rx); index++)
    {
        if (index != 0u)
        {
            Uart_DebugPutc(' ');
        }
        Test09_PrintHex8(debug->rx[index]);
    }

    Uart_DebugPuts(" crcT=");
    Test09_PrintHex8(debug->temperature_crc_calc);
    Uart_DebugPuts(" crcH=");
    Test09_PrintHex8(debug->humidity_crc_calc);
    Uart_DebugPuts("\n");
}

void Test09_Sht30_Run(void)
{
    SensorIf_Sht30DataType data;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 09_sht30_test start\n");
    Uart_DebugPuts("[INFO] chip=SHT30, addr=0x44\n");

    if (SensorIf_Sht30Init() == 0u)
    {
        Uart_DebugPuts("[FAIL] SHT30 not responding\n");
        while (1)
        {
            Test09_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[PASS] SHT30 ack\n");

    while (1)
    {
        if (SensorIf_Sht30Read(&data) != 0u)
        {
            Uart_DebugPuts("[SHT30] temp=");
            Test09_PrintSignedX100(data.temperature_c_x100);
            Uart_DebugPuts("C hum=");
            Test09_PrintSignedX100(data.humidity_rh_x100);
            Uart_DebugPuts("% rawT=");
            Uart_DebugPutHex32(data.raw_temperature);
            Uart_DebugPuts(" rawH=");
            Uart_DebugPutHex32(data.raw_humidity);
            Uart_DebugPuts("\n");
        }
        else
        {
            Test09_PrintSht30Fail();
        }

        Test09_WaitWithPowerCheck(1000u);
    }
}

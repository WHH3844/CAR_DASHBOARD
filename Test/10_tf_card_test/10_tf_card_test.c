#include "tf_card_test.h"

#include "PowerIf.h"
#include "SdIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

static uint8_t Test10_Block0[512];

static void Test10_DelayMs(uint32_t ms)
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

static void Test10_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test10_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test10_PrintHex8(uint8_t value)
{
    uint8_t nibble;

    nibble = (uint8_t)((value >> 4u) & 0x0Fu);
    Uart_DebugPutc((char)((nibble < 10u) ? ('0' + nibble) : ('A' + nibble - 10u)));
    nibble = (uint8_t)(value & 0x0Fu);
    Uart_DebugPutc((char)((nibble < 10u) ? ('0' + nibble) : ('A' + nibble - 10u)));
}

static void Test10_PrintStatus(Sdio_StatusType status)
{
    switch (status)
    {
    case SDIO_STATUS_OK:
        Uart_DebugPuts("OK");
        break;

    case SDIO_STATUS_TIMEOUT:
        Uart_DebugPuts("TIMEOUT");
        break;

    case SDIO_STATUS_CMD_TIMEOUT:
        Uart_DebugPuts("CMD_TIMEOUT");
        break;

    case SDIO_STATUS_CMD_CRC:
        Uart_DebugPuts("CMD_CRC");
        break;

    case SDIO_STATUS_UNSUPPORTED:
        Uart_DebugPuts("UNSUPPORTED");
        break;

    case SDIO_STATUS_NOT_READY:
        Uart_DebugPuts("NOT_READY");
        break;

    case SDIO_STATUS_READ_ERROR:
        Uart_DebugPuts("READ_ERROR");
        break;

    case SDIO_STATUS_PARAM:
        Uart_DebugPuts("PARAM");
        break;

    default:
        Uart_DebugPuts("UNKNOWN");
        break;
    }
}

static void Test10_PrintCardInfo(const Sdio_CardInfoType *info)
{
    Uart_DebugPuts("[INFO] type=");
    if (info->card_type == SDIO_CARD_TYPE_SDHC_SDXC)
    {
        Uart_DebugPuts("SDHC/SDXC");
    }
    else if (info->card_type == SDIO_CARD_TYPE_SDSC)
    {
        Uart_DebugPuts("SDSC");
    }
    else
    {
        Uart_DebugPuts("unknown");
    }

    Uart_DebugPuts(" rca=");
    Uart_DebugPutHex32(info->rca);
    Uart_DebugPuts(" ocr=");
    Uart_DebugPutHex32(info->ocr);
    Uart_DebugPuts("\n");

    Uart_DebugPuts("[INFO] blocks=");
    Uart_DebugPutDec(info->block_count);
    Uart_DebugPuts(" block_size=");
    Uart_DebugPutDec(info->block_size);
    Uart_DebugPuts(" capacityMB=");
    Uart_DebugPutDec(info->block_count / 2048u);
    Uart_DebugPuts("\n");
}

static void Test10_PrintBlockPreview(const uint8_t *buffer)
{
    uint8_t index;

    Uart_DebugPuts("[INFO] sector0 first16=");
    for (index = 0u; index < 16u; index++)
    {
        if (index != 0u)
        {
            Uart_DebugPutc(' ');
        }
        Test10_PrintHex8(buffer[index]);
    }
    Uart_DebugPuts("\n");

    Uart_DebugPuts("[INFO] sector0 signature=");
    Test10_PrintHex8(buffer[510]);
    Uart_DebugPutc(' ');
    Test10_PrintHex8(buffer[511]);
    Uart_DebugPuts("\n");
}

static void Test10_PrintFail(const char *stage, Sdio_StatusType status)
{
    Uart_DebugPuts("[FAIL] ");
    Uart_DebugPuts(stage);
    Uart_DebugPuts(" status=");
    Test10_PrintStatus(status);
    Uart_DebugPuts(" cmd=");
    Uart_DebugPutDec(SdIf_GetLastCommand());
    Uart_DebugPuts(" stat=");
    Uart_DebugPutHex32(SdIf_GetLastStatusRegister());
    Uart_DebugPuts("\n");
}

void Test10_TfCard_Run(void)
{
    Sdio_StatusType status;
    const Sdio_CardInfoType *info;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 10_tf_card_test start\n");
    Uart_DebugPuts("[INFO] SDIO: PC8=D0 PC9=D1 PC10=D2 PC11=D3 PC12=CLK PD2=CMD\n");
    Uart_DebugPuts("[INFO] read-only test: init card and read sector 0\n");

    status = SdIf_Init();
    if (status != SDIO_STATUS_OK)
    {
        Test10_PrintFail("TF card init", status);
        Uart_DebugPuts("[INFO] check card inserted, 3V3, CMD/DAT pull-up, CLK series resistor, socket direction\n");
        while (1)
        {
            Test10_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[PASS] TF card init\n");
    info = SdIf_GetCardInfo();
    Test10_PrintCardInfo(info);

    if (SdIf_ReadBlock0(Test10_Block0) == 0u)
    {
        Test10_PrintFail("read sector 0", SdIf_GetLastStatus());
        while (1)
        {
            Test10_WaitWithPowerCheck(500u);
        }
    }

    Uart_DebugPuts("[PASS] read sector 0\n");
    Test10_PrintBlockPreview(Test10_Block0);

    if ((Test10_Block0[510] == 0x55u) && (Test10_Block0[511] == 0xAAu))
    {
        Uart_DebugPuts("[PASS] boot sector signature 55 AA\n");
        Uart_DebugPuts("[PASS] 10_tf_card_test read-only passed\n");
    }
    else
    {
        Uart_DebugPuts("[WARN] sector0 signature is not 55 AA, raw read still finished\n");
    }

    while (1)
    {
        Uart_DebugPuts("[BOOT] TF card test alive\n");
        Test10_WaitWithPowerCheck(1000u);
    }
}

#include "can_loop_test.h"

#include "Can.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST05_TX_STD_ID             0x321u
#define TEST05_TX_TIMEOUT_LOOP       2000000u

static void Test05_DelayMs(uint32_t ms)
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

static void Test05_PrintHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    Uart_DebugPutc(hex[(value >> 4u) & 0x0Fu]);
    Uart_DebugPutc(hex[value & 0x0Fu]);
}

static void Test05_PrintData(const uint8_t *data, uint8_t len)
{
    uint8_t index;

    for (index = 0u; index < len; index++)
    {
        if (index != 0u)
        {
            Uart_DebugPutc(' ');
        }
        Test05_PrintHex8(data[index]);
    }
}

static void Test05_PrintTxResult(Can1_TxResultType result)
{
    if (result == CAN1_TX_OK)
    {
        Uart_DebugPuts("ok");
    }
    else if (result == CAN1_TX_NO_MAILBOX)
    {
        Uart_DebugPuts("no mailbox");
    }
    else if (result == CAN1_TX_TIMEOUT)
    {
        Uart_DebugPuts("timeout/no ack");
    }
    else
    {
        Uart_DebugPuts("failed");
    }
}

static void Test05_PrintCanHealth(void)
{
    Uart_DebugPuts(" tec=");
    Uart_DebugPutDec(Can1_TxErrorCount());
    Uart_DebugPuts(" rec=");
    Uart_DebugPutDec(Can1_RxErrorCount());
    Uart_DebugPuts(" err_n=");
    Uart_DebugPuts((Can1_ErrIsAsserted() != 0u) ? "low" : "high");
}

static void Test05_PollRx(void)
{
    Can_MessageType message;

    while (Can1_Read(&message) != 0u)
    {
        Uart_DebugPuts("[CAN RX] ");
        Uart_DebugPuts((message.is_extended != 0u) ? "ext id=" : "std id=");
        Uart_DebugPutHex32(message.id);
        Uart_DebugPuts((message.is_remote != 0u) ? " remote dlc=" : " data dlc=");
        Uart_DebugPutDec(message.dlc);
        Uart_DebugPuts(" bytes=");
        Test05_PrintData(message.data, message.dlc);
        Test05_PrintCanHealth();
        Uart_DebugPuts("\n");
    }
}

static void Test05_WaitWithPowerAndRx(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        Test05_PollRx();
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test05_DelayMs(10u);
        elapsed += 10u;
    }
}

void Test05_CanLoop_Run(void)
{
    uint8_t counter;
    uint8_t data[8];
    Can1_TxResultType result;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 05_can_loop_test start\n");
    Uart_DebugPuts("[INFO] CAN1: PB13=TX PB12=RX PB14=EN PB15=STB_N PG3=ERR_N\n");
    Uart_DebugPuts("[INFO] baud=");
    Uart_DebugPutDec(CAN1_TEST_BAUDRATE);
    Uart_DebugPuts(", USB-CAN use normal mode, standard frame can ack\n");

    if (Can1_Init500K() == 0u)
    {
        Uart_DebugPuts("[FAIL] CAN1 init\n");
        while (1)
        {
            Test05_WaitWithPowerAndRx(100u);
        }
    }

    Uart_DebugPuts("[PASS] CAN1 init\n");
    Uart_DebugPuts("[INFO] board sends std id=");
    Uart_DebugPutHex32(TEST05_TX_STD_ID);
    Uart_DebugPuts(" every 1s, RX accepts all frames\n");

    counter = 0u;
    while (1)
    {
        data[0] = counter;
        data[1] = (uint8_t)(counter + 1u);
        data[2] = 0x11u;
        data[3] = 0x22u;
        data[4] = 0x33u;
        data[5] = 0x44u;
        data[6] = 0x55u;
        data[7] = (uint8_t)(~counter);

        result = Can1_SendStd(TEST05_TX_STD_ID, data, 8u, TEST05_TX_TIMEOUT_LOOP);

        Uart_DebugPuts("[CAN TX] std id=");
        Uart_DebugPutHex32(TEST05_TX_STD_ID);
        Uart_DebugPuts(" dlc=8 bytes=");
        Test05_PrintData(data, 8u);
        Uart_DebugPuts(" result=");
        Test05_PrintTxResult(result);
        Test05_PrintCanHealth();
        Uart_DebugPuts("\n");

        counter++;
        Test05_WaitWithPowerAndRx(1000u);
    }
}

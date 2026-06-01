#include "Can.h"

#include "board_pins.h"

#include "gd32f4xx_can.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

#define CAN1_FILTER_START_BANK      14u
#define CAN1_ACCEPT_ALL_FILTER      14u
#define CAN1_500K_PRESCALER         5u

static void Can1_GpioInit(void)
{
    rcu_periph_clock_enable(CAN1_GPIO_CLK);
    rcu_periph_clock_enable(CAN1_CTRL_GPIO_CLK);
    rcu_periph_clock_enable(CAN1_ERR_N_GPIO_CLK);

    gpio_af_set(CAN1_TX_PORT, CAN1_GPIO_AF, CAN1_TX_PIN);
    gpio_af_set(CAN1_RX_PORT, CAN1_GPIO_AF, CAN1_RX_PIN);

    gpio_mode_set(CAN1_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, CAN1_TX_PIN);
    gpio_output_options_set(CAN1_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_TX_PIN);

    gpio_mode_set(CAN1_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, CAN1_RX_PIN);
    gpio_output_options_set(CAN1_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_RX_PIN);

    gpio_mode_set(CAN1_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CAN1_EN_PIN);
    gpio_output_options_set(CAN1_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_EN_PIN);

    gpio_mode_set(CAN1_STB_N_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CAN1_STB_N_PIN);
    gpio_output_options_set(CAN1_STB_N_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_STB_N_PIN);

    gpio_mode_set(CAN1_ERR_N_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, CAN1_ERR_N_PIN);

    /* SIT1043QT：EN 拉高使能，STB_N 拉高退出待机，进入正常收发模式。 */
    gpio_bit_set(CAN1_EN_PORT, CAN1_EN_PIN);
    gpio_bit_set(CAN1_STB_N_PORT, CAN1_STB_N_PIN);
}

uint8_t Can1_Init500K(void)
{
    can_parameter_struct can_parameter;
    can_filter_parameter_struct can_filter;

    Can1_GpioInit();

    /* GD32F4 的 CAN1 过滤器和 CAN0 共用资源，使用 CAN1 时也要打开 CAN0 时钟。 */
    rcu_periph_clock_enable(RCU_CAN0);
    rcu_periph_clock_enable(RCU_CAN1);

    can_deinit(CAN1);
    can_struct_para_init(CAN_INIT_STRUCT, &can_parameter);

    can_parameter.working_mode = CAN_NORMAL_MODE;
    can_parameter.resync_jump_width = CAN_BT_SJW_1TQ;
    can_parameter.time_segment_1 = CAN_BT_BS1_15TQ;
    can_parameter.time_segment_2 = CAN_BT_BS2_4TQ;
    can_parameter.prescaler = CAN1_500K_PRESCALER;
    can_parameter.auto_bus_off_recovery = ENABLE;
    can_parameter.auto_retrans = ENABLE;
    can_parameter.auto_wake_up = DISABLE;
    can_parameter.rec_fifo_overwrite = DISABLE;
    can_parameter.trans_fifo_order = DISABLE;
    can_parameter.time_triggered = DISABLE;

    if (can_init(CAN1, &can_parameter) != SUCCESS)
    {
        return 0u;
    }

    can1_filter_start_bank(CAN1_FILTER_START_BANK);
    can_struct_para_init(CAN_FILTER_STRUCT, &can_filter);

    /* 测试阶段先接收所有帧，方便 USB-CAN 随便发一帧就能在串口看到。 */
    can_filter.filter_number = CAN1_ACCEPT_ALL_FILTER;
    can_filter.filter_mode = CAN_FILTERMODE_MASK;
    can_filter.filter_bits = CAN_FILTERBITS_32BIT;
    can_filter.filter_fifo_number = CAN_FIFO0;
    can_filter.filter_list_high = 0x0000u;
    can_filter.filter_list_low = 0x0000u;
    can_filter.filter_mask_high = 0x0000u;
    can_filter.filter_mask_low = 0x0000u;
    can_filter.filter_enable = ENABLE;
    can_filter_init(&can_filter);

    return 1u;
}

Can1_TxResultType Can1_SendStd(uint16_t id,
                               const uint8_t *data,
                               uint8_t len,
                               uint32_t timeout_loop)
{
    can_trasnmit_message_struct tx_message;
    can_transmit_state_enum state;
    uint8_t mailbox;
    uint8_t index;

    if (len > 8u)
    {
        len = 8u;
    }

    can_struct_para_init(CAN_TX_MESSAGE_STRUCT, &tx_message);
    tx_message.tx_sfid = ((uint32_t)id & 0x7FFu);
    tx_message.tx_ff = (uint8_t)CAN_FF_STANDARD;
    tx_message.tx_ft = (uint8_t)CAN_FT_DATA;
    tx_message.tx_dlen = len;

    for (index = 0u; index < len; index++)
    {
        tx_message.tx_data[index] = data[index];
    }

    mailbox = can_message_transmit(CAN1, &tx_message);
    if (mailbox == CAN_NOMAILBOX)
    {
        return CAN1_TX_NO_MAILBOX;
    }

    while (timeout_loop-- != 0u)
    {
        state = can_transmit_states(CAN1, mailbox);
        if (state == CAN_TRANSMIT_OK)
        {
            return CAN1_TX_OK;
        }

        if (state == CAN_TRANSMIT_FAILED)
        {
            return CAN1_TX_FAILED;
        }
    }

    can_transmission_stop(CAN1, mailbox);
    return CAN1_TX_TIMEOUT;
}

uint8_t Can1_Read(Can_MessageType *message)
{
    can_receive_message_struct rx_message;
    uint8_t index;

    if (message == 0)
    {
        return 0u;
    }

    if (can_receive_message_length_get(CAN1, CAN_FIFO0) == 0u)
    {
        return 0u;
    }

    can_struct_para_init(CAN_RX_MESSAGE_STRUCT, &rx_message);
    can_message_receive(CAN1, CAN_FIFO0, &rx_message);

    message->is_extended = (rx_message.rx_ff == CAN_FF_EXTENDED) ? 1u : 0u;
    message->is_remote = (rx_message.rx_ft == CAN_FT_REMOTE) ? 1u : 0u;
    message->id = (message->is_extended != 0u) ? rx_message.rx_efid : rx_message.rx_sfid;
    message->dlc = (rx_message.rx_dlen > 8u) ? 8u : rx_message.rx_dlen;

    for (index = 0u; index < message->dlc; index++)
    {
        message->data[index] = rx_message.rx_data[index];
    }

    return 1u;
}

uint8_t Can1_ErrIsAsserted(void)
{
    return (gpio_input_bit_get(CAN1_ERR_N_PORT, CAN1_ERR_N_PIN) == RESET) ? 1u : 0u;
}

uint8_t Can1_TxErrorCount(void)
{
    return can_transmit_error_number_get(CAN1);
}

uint8_t Can1_RxErrorCount(void)
{
    return can_receive_error_number_get(CAN1);
}

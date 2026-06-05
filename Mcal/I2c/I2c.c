#include "I2c.h"

#include "Os.h"
#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_i2c.h"
#include "gd32f4xx_rcu.h"

static void I2c0_ClearErrorFlags(void)
{
    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_AERR) == SET)
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_AERR);
    }

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_BERR) == SET)
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_BERR);
    }

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_LOSTARB) == SET)
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_LOSTARB);
    }
}

static uint8_t I2c0_WaitFlagSet(i2c_flag_enum flag, uint32_t timeout_loop)
{
    while (timeout_loop-- != 0u)
    {
        if (i2c_flag_get(I2C0_BUS, flag) == SET)
        {
            return 1u;
        }

        if ((i2c_flag_get(I2C0_BUS, I2C_FLAG_AERR) == SET) ||
            (i2c_flag_get(I2C0_BUS, I2C_FLAG_BERR) == SET) ||
            (i2c_flag_get(I2C0_BUS, I2C_FLAG_LOSTARB) == SET))
        {
            return 0u;
        }
    }

    return 0u;
}

static void I2c0_WaitBusIdle(uint32_t timeout_loop)
{
    while ((timeout_loop-- != 0u) &&
           (i2c_flag_get(I2C0_BUS, I2C_FLAG_I2CBSY) == SET))
    {
    }
}

static void I2c0_Init100KUnlocked(void)
{
    rcu_periph_clock_enable(I2C0_GPIO_CLK);
    rcu_periph_clock_enable(I2C0_BUS_CLK);

    gpio_af_set(I2C0_SCL_PORT, I2C0_GPIO_AF, I2C0_SCL_PIN);
    gpio_af_set(I2C0_SDA_PORT, I2C0_GPIO_AF, I2C0_SDA_PIN);

    gpio_mode_set(I2C0_SCL_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C0_SCL_PIN);
    gpio_output_options_set(I2C0_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C0_SCL_PIN);

    gpio_mode_set(I2C0_SDA_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C0_SDA_PIN);
    gpio_output_options_set(I2C0_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C0_SDA_PIN);

    i2c_deinit(I2C0_BUS);
    i2c_clock_config(I2C0_BUS, I2C0_BUS_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0_BUS, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0x72u);
    i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
    i2c_enable(I2C0_BUS);
}

void I2c0_Init100K(void)
{
    if (Os_I2c0Lock() == 0u)
    {
        return;
    }

    I2c0_Init100KUnlocked();
    Os_I2c0Unlock();
}

static uint8_t I2c0_ProbeAddressUnlocked(uint8_t address7, uint32_t timeout_loop)
{
    uint32_t address8;

    if ((address7 < 0x08u) || (address7 > 0x77u))
    {
        return 0u;
    }

    I2c0_ClearErrorFlags();
    I2c0_WaitBusIdle(timeout_loop);

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_I2CBSY) == SET)
    {
        return 0u;
    }

    i2c_start_on_bus(I2C0_BUS);
    if (I2c0_WaitFlagSet(I2C_FLAG_SBSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    address8 = ((uint32_t)address7) << 1u;
    i2c_master_addressing(I2C0_BUS, address8, I2C_TRANSMITTER);

    while (timeout_loop-- != 0u)
    {
        if (i2c_flag_get(I2C0_BUS, I2C_FLAG_ADDSEND) == SET)
        {
            i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);
            i2c_stop_on_bus(I2C0_BUS);
            I2c0_WaitBusIdle(timeout_loop);
            return 1u;
        }

        if ((i2c_flag_get(I2C0_BUS, I2C_FLAG_AERR) == SET) ||
            (i2c_flag_get(I2C0_BUS, I2C_FLAG_BERR) == SET) ||
            (i2c_flag_get(I2C0_BUS, I2C_FLAG_LOSTARB) == SET))
        {
            break;
        }
    }

    i2c_stop_on_bus(I2C0_BUS);
    I2c0_ClearErrorFlags();
    I2c0_WaitBusIdle(timeout_loop);
    return 0u;
}

uint8_t I2c0_ProbeAddress(uint8_t address7, uint32_t timeout_loop)
{
    uint8_t result;

    if (Os_I2c0Lock() == 0u)
    {
        return 0u;
    }

    result = I2c0_ProbeAddressUnlocked(address7, timeout_loop);
    Os_I2c0Unlock();
    return result;
}

static uint8_t I2c0_WriteBytesUnlocked(uint8_t address7,
                                       const uint8_t *data,
                                       uint8_t len,
                                       uint32_t timeout_loop)
{
    uint8_t index;
    uint32_t address8;

    if ((data == 0) || (len == 0u))
    {
        return 0u;
    }

    I2c0_ClearErrorFlags();
    I2c0_WaitBusIdle(timeout_loop);

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_I2CBSY) == SET)
    {
        return 0u;
    }

    i2c_start_on_bus(I2C0_BUS);
    if (I2c0_WaitFlagSet(I2C_FLAG_SBSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    address8 = ((uint32_t)address7) << 1u;
    i2c_master_addressing(I2C0_BUS, address8, I2C_TRANSMITTER);
    if (I2c0_WaitFlagSet(I2C_FLAG_ADDSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }
    i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);

    for (index = 0u; index < len; index++)
    {
        if (I2c0_WaitFlagSet(I2C_FLAG_TBE, timeout_loop) == 0u)
        {
            i2c_stop_on_bus(I2C0_BUS);
            I2c0_ClearErrorFlags();
            return 0u;
        }

        i2c_data_transmit(I2C0_BUS, data[index]);
    }

    if (I2c0_WaitFlagSet(I2C_FLAG_BTC, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    i2c_stop_on_bus(I2C0_BUS);
    I2c0_WaitBusIdle(timeout_loop);
    I2c0_ClearErrorFlags();
    return 1u;
}

uint8_t I2c0_WriteBytes(uint8_t address7,
                        const uint8_t *data,
                        uint8_t len,
                        uint32_t timeout_loop)
{
    uint8_t result;

    if (Os_I2c0Lock() == 0u)
    {
        return 0u;
    }

    result = I2c0_WriteBytesUnlocked(address7, data, len, timeout_loop);
    Os_I2c0Unlock();
    return result;
}

static uint8_t I2c0_ReadByteAfterWriteByteUnlocked(uint8_t address7,
                                                   uint8_t write_byte,
                                                   uint8_t *read_byte,
                                                   uint32_t timeout_loop)
{
    uint32_t address8;

    if (read_byte == 0)
    {
        return 0u;
    }

    I2c0_ClearErrorFlags();
    I2c0_WaitBusIdle(timeout_loop);

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_I2CBSY) == SET)
    {
        return 0u;
    }

    i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
    i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);

    i2c_start_on_bus(I2C0_BUS);
    if (I2c0_WaitFlagSet(I2C_FLAG_SBSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    address8 = ((uint32_t)address7) << 1u;
    i2c_master_addressing(I2C0_BUS, address8, I2C_TRANSMITTER);
    if (I2c0_WaitFlagSet(I2C_FLAG_ADDSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }
    i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);

    if (I2c0_WaitFlagSet(I2C_FLAG_TBE, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    i2c_data_transmit(I2C0_BUS, write_byte);
    if (I2c0_WaitFlagSet(I2C_FLAG_BTC, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    i2c_start_on_bus(I2C0_BUS);
    if (I2c0_WaitFlagSet(I2C_FLAG_SBSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    i2c_master_addressing(I2C0_BUS, address8, I2C_RECEIVER);
    if (I2c0_WaitFlagSet(I2C_FLAG_ADDSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }
    i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);
    i2c_stop_on_bus(I2C0_BUS);

    if (I2c0_WaitFlagSet(I2C_FLAG_RBNE, timeout_loop) == 0u)
    {
        I2c0_ClearErrorFlags();
        return 0u;
    }

    *read_byte = i2c_data_receive(I2C0_BUS);
    I2c0_WaitBusIdle(timeout_loop);
    I2c0_ClearErrorFlags();
    return 1u;
}

uint8_t I2c0_ReadByteAfterWriteByte(uint8_t address7,
                                    uint8_t write_byte,
                                    uint8_t *read_byte,
                                    uint32_t timeout_loop)
{
    uint8_t result;

    if (Os_I2c0Lock() == 0u)
    {
        return 0u;
    }

    result = I2c0_ReadByteAfterWriteByteUnlocked(address7, write_byte, read_byte, timeout_loop);
    Os_I2c0Unlock();
    return result;
}

static uint8_t I2c0_ReadBytesUnlocked(uint8_t address7,
                                      uint8_t *data,
                                      uint8_t len,
                                      uint32_t timeout_loop)
{
    uint32_t address8;
    uint8_t index;
    uint8_t remain;

    if ((data == 0) || (len == 0u))
    {
        return 0u;
    }

    I2c0_ClearErrorFlags();
    I2c0_WaitBusIdle(timeout_loop);

    if (i2c_flag_get(I2C0_BUS, I2C_FLAG_I2CBSY) == SET)
    {
        return 0u;
    }

    i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
    if (len == 1u)
    {
        i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
    }
    else if (len == 2u)
    {
        i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_NEXT);
        i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
    }
    else
    {
        i2c_ack_config(I2C0_BUS, I2C_ACK_ENABLE);
    }

    i2c_start_on_bus(I2C0_BUS);
    if (I2c0_WaitFlagSet(I2C_FLAG_SBSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        return 0u;
    }

    address8 = ((uint32_t)address7) << 1u;
    i2c_master_addressing(I2C0_BUS, address8, I2C_RECEIVER);
    if (I2c0_WaitFlagSet(I2C_FLAG_ADDSEND, timeout_loop) == 0u)
    {
        i2c_stop_on_bus(I2C0_BUS);
        I2c0_ClearErrorFlags();
        i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
        i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
        return 0u;
    }

    if (len == 1u)
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);
        i2c_stop_on_bus(I2C0_BUS);

        if (I2c0_WaitFlagSet(I2C_FLAG_RBNE, timeout_loop) == 0u)
        {
            I2c0_ClearErrorFlags();
            i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
            i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
            return 0u;
        }

        data[0] = i2c_data_receive(I2C0_BUS);
    }
    else if (len == 2u)
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);
        i2c_stop_on_bus(I2C0_BUS);

        if (I2c0_WaitFlagSet(I2C_FLAG_BTC, timeout_loop) == 0u)
        {
            I2c0_ClearErrorFlags();
            i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
            i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
            return 0u;
        }

        data[0] = i2c_data_receive(I2C0_BUS);
        data[1] = i2c_data_receive(I2C0_BUS);
    }
    else
    {
        i2c_flag_clear(I2C0_BUS, I2C_FLAG_ADDSEND);

        index = 0u;
        remain = len;

        while (remain > 3u)
        {
            if (I2c0_WaitFlagSet(I2C_FLAG_RBNE, timeout_loop) == 0u)
            {
                I2c0_ClearErrorFlags();
                i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
                i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
                return 0u;
            }

            data[index] = i2c_data_receive(I2C0_BUS);
            index++;
            remain--;
        }

        /*
         * 剩余 3 字节时先等到倒数第 2 字节进入移位寄存器，
         * 再关闭 ACK，确保最后一个字节由主机 NACK 结束。
         */
        if (I2c0_WaitFlagSet(I2C_FLAG_BTC, timeout_loop) == 0u)
        {
            I2c0_ClearErrorFlags();
            i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
            i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
            return 0u;
        }

        i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
        data[index] = i2c_data_receive(I2C0_BUS);
        index++;
        remain--;

        if (I2c0_WaitFlagSet(I2C_FLAG_BTC, timeout_loop) == 0u)
        {
            I2c0_ClearErrorFlags();
            i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
            i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
            return 0u;
        }

        i2c_stop_on_bus(I2C0_BUS);
        data[index] = i2c_data_receive(I2C0_BUS);
        index++;
        remain--;
        data[index] = i2c_data_receive(I2C0_BUS);
        remain--;
    }

    I2c0_WaitBusIdle(timeout_loop);
    I2c0_ClearErrorFlags();
    i2c_ack_config(I2C0_BUS, I2C_ACK_DISABLE);
    i2c_ackpos_config(I2C0_BUS, I2C_ACKPOS_CURRENT);
    return 1u;
}

uint8_t I2c0_ReadBytes(uint8_t address7,
                       uint8_t *data,
                       uint8_t len,
                       uint32_t timeout_loop)
{
    uint8_t result;

    if (Os_I2c0Lock() == 0u)
    {
        return 0u;
    }

    result = I2c0_ReadBytesUnlocked(address7, data, len, timeout_loop);
    Os_I2c0Unlock();
    return result;
}

uint8_t I2c0_SclIsHigh(void)
{
    return (gpio_input_bit_get(I2C0_SCL_PORT, I2C0_SCL_PIN) == SET) ? 1u : 0u;
}

uint8_t I2c0_SdaIsHigh(void)
{
    return (gpio_input_bit_get(I2C0_SDA_PORT, I2C0_SDA_PIN) == SET) ? 1u : 0u;
}

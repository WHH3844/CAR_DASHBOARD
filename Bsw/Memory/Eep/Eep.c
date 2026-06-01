#include "Eep.h"

#include "I2c.h"

#include "gd32f4xx.h"

#define EEP_BASE_ADDRESS             0x50u
#define EEP_BLOCK_MASK               0x07u
#define EEP_TIMEOUT_LOOP             50000u
#define EEP_WRITE_READY_RETRY        40u

static void Eep_DelayMs(uint32_t ms)
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

static uint8_t Eep_DeviceAddress(uint16_t memory_address)
{
    return (uint8_t)(EEP_BASE_ADDRESS |
                     ((memory_address >> 8u) & EEP_BLOCK_MASK));
}

static uint8_t Eep_WaitReady(uint8_t device_address)
{
    uint8_t retry;

    for (retry = 0u; retry < EEP_WRITE_READY_RETRY; retry++)
    {
        if (I2c0_ProbeAddress(device_address, EEP_TIMEOUT_LOOP) != 0u)
        {
            return 1u;
        }

        Eep_DelayMs(1u);
    }

    return 0u;
}

void Eep_Init(void)
{
    I2c0_Init100K();
}

uint8_t Eep_WriteByte(uint16_t address, uint8_t data)
{
    uint8_t buffer[2];
    uint8_t device_address;

    if (address >= EEP_FT24C16A_SIZE_BYTES)
    {
        return 0u;
    }

    device_address = Eep_DeviceAddress(address);
    buffer[0] = (uint8_t)(address & 0xFFu);
    buffer[1] = data;

    if (I2c0_WriteBytes(device_address, buffer, 2u, EEP_TIMEOUT_LOOP) == 0u)
    {
        return 0u;
    }

    return Eep_WaitReady(device_address);
}

uint8_t Eep_ReadByte(uint16_t address, uint8_t *data)
{
    uint8_t device_address;
    uint8_t word_address;

    if ((address >= EEP_FT24C16A_SIZE_BYTES) || (data == 0))
    {
        return 0u;
    }

    device_address = Eep_DeviceAddress(address);
    word_address = (uint8_t)(address & 0xFFu);

    return I2c0_ReadByteAfterWriteByte(device_address,
                                       word_address,
                                       data,
                                       EEP_TIMEOUT_LOOP);
}

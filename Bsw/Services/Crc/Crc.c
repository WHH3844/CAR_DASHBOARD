#include "Crc.h"

#define CRC16_POLY_CCITT    0x1021u

uint16_t Crc_CalculateCrc16(const uint8_t *data, uint16_t length, uint16_t start_value)
{
    uint16_t crc;
    uint16_t index;
    uint8_t bit;

    if (data == 0)
    {
        return start_value;
    }

    crc = start_value;
    for (index = 0u; index < length; index++)
    {
        crc ^= (uint16_t)((uint16_t)data[index] << 8u);

        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16_t)((crc << 1u) ^ CRC16_POLY_CCITT);
            }
            else
            {
                crc = (uint16_t)(crc << 1u);
            }
        }
    }

    return crc;
}

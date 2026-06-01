#include "SensorIf.h"

#include "I2c.h"

#include "gd32f4xx.h"

#define SENSORIF_SHT30_ADDR          0x44u
#define SENSORIF_SHT30_TIMEOUT       50000u

static SensorIf_Sht30DebugType SensorIf_Sht30Debug;

static void SensorIf_DelayMs(uint32_t ms)
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

static uint8_t SensorIf_Sht30Crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc;
    uint8_t index;
    uint8_t bit;

    crc = 0xFFu;
    for (index = 0u; index < len; index++)
    {
        crc ^= data[index];
        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8_t)((crc << 1u) ^ 0x31u);
            }
            else
            {
                crc <<= 1u;
            }
        }
    }

    return crc;
}

static void SensorIf_Sht30ClearDebug(void)
{
    uint8_t index;

    for (index = 0u; index < sizeof(SensorIf_Sht30Debug.rx); index++)
    {
        SensorIf_Sht30Debug.rx[index] = 0u;
    }

    SensorIf_Sht30Debug.temperature_crc_calc = 0u;
    SensorIf_Sht30Debug.humidity_crc_calc = 0u;
    SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_OK;
}

uint8_t SensorIf_Sht30Init(void)
{
    I2c0_Init100K();
    return I2c0_ProbeAddress(SENSORIF_SHT30_ADDR, SENSORIF_SHT30_TIMEOUT);
}

uint8_t SensorIf_Sht30Read(SensorIf_Sht30DataType *data)
{
    uint8_t command[2];
    uint8_t rx[6];
    uint8_t index;
    uint8_t temperature_crc;
    uint8_t humidity_crc;
    uint32_t temp_calc;
    uint32_t hum_calc;

    SensorIf_Sht30ClearDebug();

    if (data == 0)
    {
        SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_PARAM;
        return 0u;
    }

    /* 单次测量，高重复性，关闭 clock stretching。 */
    command[0] = 0x24u;
    command[1] = 0x00u;

    if (I2c0_WriteBytes(SENSORIF_SHT30_ADDR,
                        command,
                        2u,
                        SENSORIF_SHT30_TIMEOUT) == 0u)
    {
        SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_WRITE_FAIL;
        return 0u;
    }

    SensorIf_DelayMs(60u);

    if (I2c0_ReadBytes(SENSORIF_SHT30_ADDR,
                       rx,
                       sizeof(rx),
                       SENSORIF_SHT30_TIMEOUT) == 0u)
    {
        SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_READ_FAIL;
        return 0u;
    }

    for (index = 0u; index < sizeof(rx); index++)
    {
        SensorIf_Sht30Debug.rx[index] = rx[index];
    }

    temperature_crc = SensorIf_Sht30Crc8(&rx[0], 2u);
    humidity_crc = SensorIf_Sht30Crc8(&rx[3], 2u);
    SensorIf_Sht30Debug.temperature_crc_calc = temperature_crc;
    SensorIf_Sht30Debug.humidity_crc_calc = humidity_crc;

    if (temperature_crc != rx[2])
    {
        SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_TEMP_CRC_FAIL;
        return 0u;
    }

    if (humidity_crc != rx[5])
    {
        SensorIf_Sht30Debug.status = SENSORIF_SHT30_STATUS_HUM_CRC_FAIL;
        return 0u;
    }

    data->raw_temperature = (uint16_t)(((uint16_t)rx[0] << 8u) | rx[1]);
    data->raw_humidity = (uint16_t)(((uint16_t)rx[3] << 8u) | rx[4]);

    temp_calc = (uint32_t)data->raw_temperature * 17500u;
    data->temperature_c_x100 = (int32_t)(-4500 + (int32_t)(temp_calc / 65535u));

    hum_calc = (uint32_t)data->raw_humidity * 10000u;
    data->humidity_rh_x100 = (int32_t)(hum_calc / 65535u);

    return 1u;
}

SensorIf_Sht30StatusType SensorIf_Sht30GetLastStatus(void)
{
    return SensorIf_Sht30Debug.status;
}

const SensorIf_Sht30DebugType *SensorIf_Sht30GetDebug(void)
{
    return &SensorIf_Sht30Debug;
}

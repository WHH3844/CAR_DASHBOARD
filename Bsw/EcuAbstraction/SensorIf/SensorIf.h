#ifndef SENSORIF_H
#define SENSORIF_H

#include <stdint.h>

typedef struct
{
    int32_t temperature_c_x100;
    int32_t humidity_rh_x100;
    uint16_t raw_temperature;
    uint16_t raw_humidity;
} SensorIf_Sht30DataType;

typedef enum
{
    SENSORIF_SHT30_STATUS_OK = 0u,
    SENSORIF_SHT30_STATUS_PARAM = 1u,
    SENSORIF_SHT30_STATUS_WRITE_FAIL = 2u,
    SENSORIF_SHT30_STATUS_READ_FAIL = 3u,
    SENSORIF_SHT30_STATUS_TEMP_CRC_FAIL = 4u,
    SENSORIF_SHT30_STATUS_HUM_CRC_FAIL = 5u
} SensorIf_Sht30StatusType;

typedef struct
{
    uint8_t rx[6];
    uint8_t temperature_crc_calc;
    uint8_t humidity_crc_calc;
    SensorIf_Sht30StatusType status;
} SensorIf_Sht30DebugType;

uint8_t SensorIf_Sht30Init(void);
uint8_t SensorIf_Sht30Read(SensorIf_Sht30DataType *data);
SensorIf_Sht30StatusType SensorIf_Sht30GetLastStatus(void);
const SensorIf_Sht30DebugType *SensorIf_Sht30GetDebug(void);

#endif /* SENSORIF_H */

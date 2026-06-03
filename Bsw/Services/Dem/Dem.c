#include "Dem.h"

#include "LogM.h"
#include "NvM.h"

#define DEM_STATUS_TEST_FAILED              0x01u
#define DEM_STATUS_CONFIRMED_DTC            0x08u
#define DEM_STATUS_WARNING_INDICATOR        0x80u
#define DEM_NVM_BYTES_PER_EVENT             3u
#define DEM_NVM_PAYLOAD_SIZE                (DEM_EVENT_COUNT * DEM_NVM_BYTES_PER_EVENT)

static const Dem_EventConfigType Dem_EventConfigs[DEM_EVENT_COUNT] =
{
    {DEM_EVENT_LOW_SUPPLY_VOLTAGE,      0x010001u, 3u, 1u},
    {DEM_EVENT_HIGH_SUPPLY_VOLTAGE,     0x010002u, 3u, 1u},
    {DEM_EVENT_SDRAM_INIT_FAILED,       0x020001u, 3u, 1u},
    {DEM_EVENT_SDRAM_RW_TEST_FAILED,    0x020002u, 3u, 1u},
    {DEM_EVENT_LCD_INIT_FAILED,         0x030001u, 3u, 1u},
    {DEM_EVENT_FRAMEBUFFER_ABNORMAL,    0x030002u, 2u, 1u},
    {DEM_EVENT_CAN_BUS_OFF,             0x040001u, 3u, 1u},
    {DEM_EVENT_CAN_RX_TIMEOUT,          0x040002u, 2u, 1u},
    {DEM_EVENT_CAN_TRCV_ERROR,          0x040003u, 2u, 1u},
    {DEM_EVENT_EEPROM_COMM_FAILED,      0x050001u, 2u, 0u},
    {DEM_EVENT_EEPROM_CRC_FAILED,       0x050002u, 2u, 0u},
    {DEM_EVENT_RTC_COMM_FAILED,         0x060001u, 1u, 0u},
    {DEM_EVENT_RTC_TIME_INVALID,        0x060002u, 1u, 0u},
    {DEM_EVENT_SHT30_COMM_FAILED,       0x070001u, 1u, 0u},
    {DEM_EVENT_TF_CARD_MOUNT_FAILED,    0x080001u, 1u, 0u},
    {DEM_EVENT_POWER_HOLD_FAILED,       0x090001u, 3u, 1u},
    {DEM_EVENT_KEY_STUCK,               0x0A0001u, 1u, 0u},
    {DEM_EVENT_BUZZER_CONTROL_FAILED,   0x0B0001u, 1u, 0u},
    {DEM_EVENT_DISPLAY_TASK_OVERLOAD,   0x0C0001u, 2u, 0u},
    {DEM_EVENT_WATCHDOG_RESET_DETECTED, 0x0D0001u, 2u, 1u}
};

static uint8_t Dem_Status[DEM_EVENT_COUNT];
static uint16_t Dem_OccurrenceCounter[DEM_EVENT_COUNT];
static uint8_t Dem_Dirty;

static void Dem_LoadFromNvM(void)
{
    uint8_t payload[DEM_NVM_PAYLOAD_SIZE];
    uint8_t index;
    uint16_t pos;

    if (NvM_ReadBlock(NVM_BLOCK_DEM_STATUS, payload, sizeof(payload)) != E_OK)
    {
        for (index = 0u; index < DEM_EVENT_COUNT; index++)
        {
            Dem_Status[index] = 0u;
            Dem_OccurrenceCounter[index] = 0u;
        }
        Dem_Dirty = 1u;
        return;
    }

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        pos = (uint16_t)(index * DEM_NVM_BYTES_PER_EVENT);
        Dem_Status[index] = payload[pos];
        Dem_OccurrenceCounter[index] = (uint16_t)((uint16_t)payload[pos + 1u] |
                                                  ((uint16_t)payload[pos + 2u] << 8u));
    }
}

static void Dem_SaveToNvM(void)
{
    uint8_t payload[DEM_NVM_PAYLOAD_SIZE];
    uint8_t index;
    uint16_t pos;

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        pos = (uint16_t)(index * DEM_NVM_BYTES_PER_EVENT);
        payload[pos] = Dem_Status[index];
        payload[pos + 1u] = (uint8_t)(Dem_OccurrenceCounter[index] & 0xFFu);
        payload[pos + 2u] = (uint8_t)((Dem_OccurrenceCounter[index] >> 8u) & 0xFFu);
    }

    if (NvM_WriteBlock(NVM_BLOCK_DEM_STATUS, payload, sizeof(payload)) == E_OK)
    {
        Dem_Dirty = 0u;
    }
}

void Dem_Init(void)
{
    Dem_LoadFromNvM();
    LogM_Info("Dem init ok");
}

void Dem_MainFunction(uint32_t tick_ms)
{
    static uint32_t last_save_ms;

    if ((Dem_Dirty != 0u) && ((tick_ms - last_save_ms) >= 1000u))
    {
        last_save_ms = tick_ms;
        Dem_SaveToNvM();
    }
}

void Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status)
{
    uint8_t old_status;

    if (event_id >= DEM_EVENT_COUNT)
    {
        return;
    }

    old_status = Dem_Status[event_id];

    if (status == DEM_EVENT_STATUS_FAILED)
    {
        /*
         * 从 passed -> failed 的边沿才增加发生次数。
         * 这样一个持续故障不会在每个调度周期疯狂刷 EEPROM。
         */
        if ((old_status & DEM_STATUS_TEST_FAILED) == 0u)
        {
            if (Dem_OccurrenceCounter[event_id] < 0xFFFFu)
            {
                Dem_OccurrenceCounter[event_id]++;
            }
        }

        Dem_Status[event_id] |= DEM_STATUS_TEST_FAILED | DEM_STATUS_CONFIRMED_DTC;
        if (Dem_EventConfigs[event_id].lamp_flag != 0u)
        {
            Dem_Status[event_id] |= DEM_STATUS_WARNING_INDICATOR;
        }
    }
    else
    {
        Dem_Status[event_id] &= (uint8_t)(~DEM_STATUS_TEST_FAILED);
    }

    if (old_status != Dem_Status[event_id])
    {
        Dem_Dirty = 1u;
    }
}

uint8_t Dem_GetDtcCountByStatusMask(uint8_t status_mask)
{
    uint8_t index;
    uint8_t count;

    count = 0u;
    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        if ((Dem_Status[index] & status_mask) != 0u)
        {
            count++;
        }
    }

    return count;
}

Std_ReturnType Dem_GetFirstDtcByStatusMask(uint8_t status_mask, Dem_DtcRecordType *record)
{
    uint8_t index;

    if (record == 0)
    {
        return E_NOT_OK;
    }

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        if ((Dem_Status[index] & status_mask) != 0u)
        {
            record->dtc = Dem_EventConfigs[index].dtc;
            record->status = Dem_Status[index];
            record->occurrence_counter = Dem_OccurrenceCounter[index];
            return E_OK;
        }
    }

    return E_NOT_OK;
}

uint8_t Dem_GetConfirmedDtcCount(void)
{
    return Dem_GetDtcCountByStatusMask(DEM_STATUS_CONFIRMED_DTC);
}

void Dem_ClearAllDtc(void)
{
    uint8_t index;

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        Dem_Status[index] = 0u;
        Dem_OccurrenceCounter[index] = 0u;
    }

    Dem_Dirty = 1u;
    Dem_SaveToNvM();
}

void Dem_SaveNow(void)
{
    if (Dem_Dirty != 0u)
    {
        Dem_SaveToNvM();
    }
}
#include "Dem.h"

#include "NvM.h"

#define DEM_STATUS_TEST_FAILED       0x01u
#define DEM_STATUS_CONFIRMED_DTC     0x08u
#define DEM_NVM_BYTES_PER_EVENT      3u

static const Dem_EventConfigType Dem_EventConfig[DEM_EVENT_COUNT] =
{
    { DEM_EVENT_LOW_SUPPLY_VOLTAGE,      0x010001u, 3u, 1u },
    { DEM_EVENT_HIGH_SUPPLY_VOLTAGE,     0x010002u, 3u, 1u },
    { DEM_EVENT_SDRAM_INIT_FAILED,       0x020001u, 3u, 1u },
    { DEM_EVENT_SDRAM_RW_TEST_FAILED,    0x020002u, 3u, 1u },
    { DEM_EVENT_LCD_INIT_FAILED,         0x030001u, 3u, 1u },
    { DEM_EVENT_FRAMEBUFFER_ABNORMAL,    0x030002u, 2u, 1u },
    { DEM_EVENT_CAN_BUS_OFF,             0x040001u, 3u, 1u },
    { DEM_EVENT_CAN_RX_TIMEOUT,          0x040002u, 2u, 1u },
    { DEM_EVENT_CAN_TRCV_ERROR,          0x040003u, 2u, 1u },
    { DEM_EVENT_EEPROM_COMM_FAILED,      0x050001u, 2u, 0u },
    { DEM_EVENT_EEPROM_CRC_FAILED,       0x050002u, 2u, 0u },
    { DEM_EVENT_RTC_COMM_FAILED,         0x060001u, 1u, 0u },
    { DEM_EVENT_RTC_TIME_INVALID,        0x060002u, 1u, 0u },
    { DEM_EVENT_SHT30_COMM_FAILED,       0x070001u, 1u, 0u },
    { DEM_EVENT_TF_CARD_MOUNT_FAILED,    0x080001u, 1u, 0u },
    { DEM_EVENT_POWER_HOLD_FAILED,       0x090001u, 3u, 1u },
    { DEM_EVENT_KEY_STUCK,               0x0A0001u, 1u, 0u },
    { DEM_EVENT_BUZZER_CONTROL_FAILED,   0x0B0001u, 1u, 0u },
    { DEM_EVENT_DISPLAY_TASK_OVERLOAD,   0x0C0001u, 2u, 0u },
    { DEM_EVENT_WATCHDOG_RESET_DETECTED, 0x0D0001u, 2u, 1u }
};

static uint8_t Dem_Status[DEM_EVENT_COUNT];
static uint16_t Dem_OccurrenceCounter[DEM_EVENT_COUNT];

void Dem_Init(void)
{
    uint8_t index;

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        Dem_Status[index] = 0u;
        Dem_OccurrenceCounter[index] = 0u;
    }
}

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status)
{
    uint8_t old_status;

    if (event_id >= DEM_EVENT_COUNT)
    {
        return E_NOT_OK;
    }

    old_status = Dem_Status[event_id];

    if (status == DEM_EVENT_STATUS_FAILED)
    {
        /*
         * 从 passed -> failed 的边沿才增加发生次数。
         * 这样周期性监控不会每 10ms 把计数刷爆。
         */
        if ((old_status & DEM_STATUS_TEST_FAILED) == 0u)
        {
            if (Dem_OccurrenceCounter[event_id] < 0xFFFFu)
            {
                Dem_OccurrenceCounter[event_id]++;
            }
        }

        Dem_Status[event_id] = DEM_STATUS_TEST_FAILED | DEM_STATUS_CONFIRMED_DTC;
        NvM_MarkDemDirty();
    }
    else
    {
        Dem_Status[event_id] = 0u;
    }

    return E_OK;
}

uint8_t Dem_GetEventStatusByte(Dem_EventIdType event_id)
{
    if (event_id >= DEM_EVENT_COUNT)
    {
        return 0u;
    }

    return Dem_Status[event_id];
}

uint8_t Dem_GetFailedDtcCount(uint8_t status_mask)
{
    uint8_t index;
    uint8_t count;

    count = 0u;
    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        if ((Dem_Status[index] & status_mask) != 0u)
        {
            count++;
        }
    }

    return count;
}

Std_ReturnType Dem_GetFirstFailedDtc(uint8_t status_mask, Dem_DtcStatusType *dtc_status)
{
    uint8_t index;

    if (dtc_status == 0)
    {
        return E_NOT_OK;
    }

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        if ((Dem_Status[index] & status_mask) != 0u)
        {
            dtc_status->dtc = Dem_EventConfig[index].dtc;
            dtc_status->status = Dem_Status[index];
            dtc_status->occurrence_counter = Dem_OccurrenceCounter[index];
            return E_OK;
        }
    }

    dtc_status->dtc = 0u;
    dtc_status->status = 0u;
    dtc_status->occurrence_counter = 0u;
    return E_NOT_OK;
}

Std_ReturnType Dem_ClearAllDtc(void)
{
    uint8_t index;

    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        Dem_Status[index] = 0u;
        Dem_OccurrenceCounter[index] = 0u;
    }

    NvM_MarkDemDirty();
    return E_OK;
}

uint8_t Dem_GetNvMData(uint8_t *buffer, uint16_t length)
{
    uint8_t index;
    uint16_t pos;

    if ((buffer == 0) || (length < (DEM_EVENT_COUNT * DEM_NVM_BYTES_PER_EVENT)))
    {
        return 0u;
    }

    pos = 0u;
    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        buffer[pos] = Dem_Status[index];
        buffer[pos + 1u] = (uint8_t)(Dem_OccurrenceCounter[index] & 0xFFu);
        buffer[pos + 2u] = (uint8_t)((Dem_OccurrenceCounter[index] >> 8u) & 0xFFu);
        pos = (uint16_t)(pos + DEM_NVM_BYTES_PER_EVENT);
    }

    return 1u;
}

void Dem_LoadNvMData(const uint8_t *buffer, uint16_t length)
{
    uint8_t index;
    uint16_t pos;

    if ((buffer == 0) || (length < (DEM_EVENT_COUNT * DEM_NVM_BYTES_PER_EVENT)))
    {
        return;
    }

    pos = 0u;
    for (index = 0u; index < DEM_EVENT_COUNT; index++)
    {
        Dem_Status[index] = buffer[pos];
        Dem_OccurrenceCounter[index] = (uint16_t)((uint16_t)buffer[pos + 1u] |
                                                  ((uint16_t)buffer[pos + 2u] << 8u));
        pos = (uint16_t)(pos + DEM_NVM_BYTES_PER_EVENT);
    }
}

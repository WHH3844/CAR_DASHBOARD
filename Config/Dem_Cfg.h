#ifndef DEM_CFG_H
#define DEM_CFG_H

#include <stdint.h>

/*
 * Dem 事件 ID 和诊断需求矩阵 08.DTCList 对齐。
 * 第一版实现状态、发生次数和 DTC 查询；快照/扩展数据先保留接口方向。
 */
typedef enum
{
    DEM_EVENT_LOW_SUPPLY_VOLTAGE = 0u,
    DEM_EVENT_HIGH_SUPPLY_VOLTAGE,
    DEM_EVENT_SDRAM_INIT_FAILED,
    DEM_EVENT_SDRAM_RW_TEST_FAILED,
    DEM_EVENT_LCD_INIT_FAILED,
    DEM_EVENT_FRAMEBUFFER_ABNORMAL,
    DEM_EVENT_CAN_BUS_OFF,
    DEM_EVENT_CAN_RX_TIMEOUT,
    DEM_EVENT_CAN_TRCV_ERROR,
    DEM_EVENT_EEPROM_COMM_FAILED,
    DEM_EVENT_EEPROM_CRC_FAILED,
    DEM_EVENT_RTC_COMM_FAILED,
    DEM_EVENT_RTC_TIME_INVALID,
    DEM_EVENT_SHT30_COMM_FAILED,
    DEM_EVENT_TF_CARD_MOUNT_FAILED,
    DEM_EVENT_POWER_HOLD_FAILED,
    DEM_EVENT_KEY_STUCK,
    DEM_EVENT_BUZZER_CONTROL_FAILED,
    DEM_EVENT_DISPLAY_TASK_OVERLOAD,
    DEM_EVENT_WATCHDOG_RESET_DETECTED,
    DEM_EVENT_COUNT
} Dem_EventIdType;

typedef struct
{
    Dem_EventIdType event_id;
    uint32_t dtc;
    uint8_t level;
    uint8_t lamp_flag;
    /* 连续失败/通过样本达到阈值后才改变 testFailed，防止一次抖动即确认。 */
    uint8_t failed_threshold;
    uint8_t passed_threshold;
} Dem_EventConfigType;

#endif /* DEM_CFG_H */

#ifndef RTE_SIGNAL_H
#define RTE_SIGNAL_H

#include "RtcIf.h"
#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    /* 无待处理按键事件。 */
    RTE_KEY_EVENT_NONE = 0u,
    /* 三个用户按键的短按事件，当前由 App_Key 的按下沿产生。 */
    RTE_KEY_EVENT_KEY1_SHORT,
    RTE_KEY_EVENT_KEY2_SHORT,
    RTE_KEY_EVENT_KEY3_SHORT,
    /* 电源键长按预留事件；当前关机路径由 App_Power 独立管理。 */
    RTE_KEY_EVENT_POWER_LONG
} Rte_KeyEventType;

typedef struct
{
    /* 车速，单位 0.1 km/h；例如 1200 表示 120.0 km/h。 */
    uint16_t vehicle_speed_kph_x10;
    /* 发动机转速，单位 rpm。 */
    uint16_t engine_rpm;
    /* 燃油百分比，0~100。 */
    uint8_t fuel_percent;
    /* 冷却液温度，单位摄氏度。 */
    int16_t coolant_temp_c;
    /* 外部环境温度，单位摄氏度，通常来自 CAN 车身/空调报文。 */
    int16_t outdoor_temp_c;
    /* 电池电压，单位 mV。 */
    uint16_t battery_mv;
    /* 点火状态和档位原始枚举，第一版先透传给显示/日志。 */
    uint8_t ignition_status;
    uint8_t gear_position;
    /* 车身告警位图，具体 bit 定义跟随 CAN 矩阵。 */
    uint8_t warning_flags;
    /* CAN 输入有效位，由 Rte_Update_CanValidity 根据最近接收时间维护。 */
    uint8_t can_ems_valid;
    uint8_t can_body_valid;
    /* 仪表本地计算出的报警状态，例如超速/超转速。 */
    uint8_t alarm_active;
    /* 用户是否静音报警蜂鸣器。 */
    uint8_t buzzer_muted;
    /* 是否使用本地模拟数据覆盖动力域车速/转速。 */
    uint8_t simulation_mode;
} Rte_DashboardDataType;

typedef struct
{
    /* SHT30 温度，单位 0.01 摄氏度。 */
    int32_t temperature_c_x100;
    /* SHT30 相对湿度，单位 0.01 %RH。 */
    int32_t humidity_rh_x100;
    /* 传感器数据有效位，I2C 失败或初始化失败时为 0。 */
    uint8_t valid;
} Rte_EnvironmentDataType;

/* 初始化所有 RTE 信号快照为安全默认值。 */
void Rte_Signal_Init(void);

/* 写入动力域数据，同时记录最近一次 0x321 接收时间。 */
Std_ReturnType Rte_Write_Powertrain(uint16_t speed_kph_x10,
                                    uint16_t rpm,
                                    uint8_t fuel_percent,
                                    int16_t coolant_temp_c,
                                    int16_t outdoor_temp_c,
                                    uint16_t battery_mv,
                                    uint32_t tick_ms);
/* 写入车身状态数据，同时记录最近一次 0x322 接收时间。 */
Std_ReturnType Rte_Write_BodyStatus(uint8_t ignition_status,
                                    uint8_t gear_position,
                                    uint8_t warning_flags,
                                    uint32_t tick_ms);
/* 写入环境传感器快照。 */
Std_ReturnType Rte_Write_Environment(const Rte_EnvironmentDataType *environment);

/* 写入 RTC 时间和有效位，time 为空时会清除有效标志并返回 E_NOT_OK。 */
Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time, uint8_t valid);

/* 写入最近一次按键事件。 */
Std_ReturnType Rte_Write_KeyEvent(Rte_KeyEventType event);

/* 写入背光等级，超过 100 会被钳位。 */
Std_ReturnType Rte_Write_BacklightLevel(uint8_t level);

/* 写入蜂鸣器静音状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_BuzzerMuted(uint8_t muted);

/* 写入模拟模式状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_SimulationMode(uint8_t enabled);

/* 写入报警状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_AlarmActive(uint8_t active);

/* 清除车速/转速和动力域有效位，保留其它显示状态。 */
void Rte_Clear_DashboardValues(void);

/* 根据 CAN 超时阈值维护 can_ems_valid/can_body_valid。 */
void Rte_Update_CanValidity(uint32_t tick_ms);

/* 读取仪表数据快照。 */
Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data);

/* 读取环境数据快照。 */
Std_ReturnType Rte_Read_Environment(Rte_EnvironmentDataType *environment);

/* 读取 RTC 时间和有效位。 */
Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time, uint8_t *valid);

/* 读取但不清除最近一次按键事件。 */
Std_ReturnType Rte_Read_KeyEvent(Rte_KeyEventType *event);

/* 读取并清除最近一次按键事件，适合只能消费一次的业务动作。 */
Std_ReturnType Rte_Take_KeyEvent(Rte_KeyEventType *event);

/* 读取当前背光等级。 */
Std_ReturnType Rte_Read_BacklightLevel(uint8_t *level);

/* 返回最近一次动力域报文写入 RTE 的时间戳。 */
uint32_t Rte_GetLastPowertrainRxTick(void);

/* 快捷读取动力域 CAN 有效位。 */
uint8_t Rte_IsPowertrainValid(void);

#endif /* RTE_SIGNAL_H */

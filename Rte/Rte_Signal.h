#ifndef RTE_SIGNAL_H
#define RTE_SIGNAL_H

#include "RtcIf.h"
#include "Std_Types.h"

#include <stdint.h>

#define RTE_POWERTRAIN_VALID_SPEED          0x01u
#define RTE_POWERTRAIN_VALID_RPM            0x02u
#define RTE_POWERTRAIN_VALID_FUEL           0x04u
#define RTE_POWERTRAIN_VALID_COOLANT        0x08u
#define RTE_POWERTRAIN_VALID_OUTDOOR        0x10u
#define RTE_POWERTRAIN_VALID_BATTERY        0x20u
#define RTE_POWERTRAIN_VALID_ALL            0x3Fu

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
    /* 0x321 六个信号分别维护有效性，报文 freshness 不能替代信号有效性。 */
    uint8_t vehicle_speed_valid;
    uint8_t engine_rpm_valid;
    uint8_t fuel_percent_valid;
    uint8_t coolant_temp_valid;
    uint8_t outdoor_temp_valid;
    uint8_t battery_voltage_valid;
    /* 点火状态和档位原始枚举，第一版先透传给显示/日志。 */
    uint8_t ignition_status;
    uint8_t gear_position;
    /* 0x322 完整车身状态。door_open_mask bit0..3=FL/FR/RL/RR。 */
    uint8_t door_open_mask;
    uint8_t driver_seatbelt;
    uint8_t low_beam;
    uint8_t high_beam;
    uint8_t turn_signal;
    uint8_t parking_brake;
    uint8_t washer_fluid_low;
    uint8_t ambient_light;
    uint8_t ambient_light_valid;
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
    /* 长按电源键后由 App_Power 置位，0x325 发送关机请求。 */
    uint8_t shutdown_request;
} Rte_DashboardDataType;

typedef struct
{
    uint8_t ignition_status;
    uint8_t gear_position;
    uint8_t door_open_mask;
    uint8_t driver_seatbelt;
    uint8_t low_beam;
    uint8_t high_beam;
    uint8_t turn_signal;
    uint8_t parking_brake;
    uint8_t warning_flags;
    uint8_t washer_fluid_low;
    uint8_t ambient_light;
    uint8_t ambient_light_valid;
} Rte_BodyStatusType;

typedef struct
{
    /* SHT30 温度，单位 0.01 摄氏度。 */
    int32_t temperature_c_x100;
    /* SHT30 相对湿度，单位 0.01 %RH。 */
    int32_t humidity_rh_x100;
    /* 传感器数据有效位，I2C 失败或初始化失败时为 0。 */
    uint8_t valid;
} Rte_EnvironmentDataType;

typedef struct
{
    /* 四轮胎压，单位 0.01 bar；例如 275 表示 2.75 bar。 */
    uint16_t pressure_bar_x100[4];
    /* 四轮胎温，单位摄氏度。 */
    int16_t temperature_c[4];
    /* 每个轮位的压力/温度有效性；bit0..3 warning_mask 对应 FL/FR/RL/RR。 */
    uint8_t pressure_valid[4];
    uint8_t temperature_valid[4];
    uint8_t warning_mask;
    /* TPMS 报文有效位，由最近一次 0x323 接收时间维护。 */
    uint8_t valid;
} Rte_TpmsDataType;

typedef struct
{
    /* 主题、语言、单位制按 0x324 Byte0 的 bit 定义保存。 */
    uint8_t theme_mode;
    uint8_t language;
    uint8_t unit_mode;
    /* 蜂鸣器报警音量目标值，0~100。当前蜂鸣器硬件只实现开关，先保存配置。 */
    uint8_t warning_volume;
    /* 续航里程，单位 km。 */
    uint16_t driving_range_km;
    /* 外部时间同步字段，当前只保存时/分和有效位。 */
    uint8_t datetime_valid;
    uint8_t time_hour;
    uint8_t time_minute;
    /* 合法远端 0x324 已接收且未超时。NvM 始终是上电基线。 */
    uint8_t remote_valid;
} Rte_ConfigDataType;

typedef struct
{
    /* 0x329 应用状态。 */
    uint8_t interface_version;
    uint8_t version_compatible;
    uint8_t input_mode;
    uint8_t power_mode;
    uint8_t health_state;
    uint8_t remote_fault_present;
    uint8_t remote_dtc_count;
    uint8_t domain_status_flags;
    uint16_t last_remote_fault_id;
    uint8_t alive_counter;
    uint8_t alive_stalled;
    uint8_t status_valid;
    uint8_t application_stale;
    /* 0x441 NM 在线状态，与 0x329 应用健康独立。 */
    uint8_t nm_online;
    uint8_t nm_node_address;
    uint8_t nm_repeat_status;
    uint8_t nm_power_on_request;
    uint8_t nm_diag_request;
} Rte_ControlDomainStatusType;

typedef struct
{
    /* 0x327 KeyCode：0 None, 1 KEY1, 2 KEY2, 3 KEY3, 4 POWER。 */
    uint8_t key_code;
    /* 0x327 KeyAction：1 ShortPress, 2 LongPress, 3 DoubleClick。 */
    uint8_t key_action;
    /* 事件计数器，接收端可用来判断是否漏事件。 */
    uint8_t event_counter;
    uint8_t power_key_long_press;
    uint8_t shutdown_confirm;
} Rte_UserInputEventType;

/* 初始化所有 RTE 信号快照为安全默认值。 */
void Rte_Signal_Init(void);

/* 写入动力域数据，同时记录最近一次 0x321 接收时间。 */
Std_ReturnType Rte_Write_Powertrain(uint16_t speed_kph_x10,
                                    uint16_t rpm,
                                    uint8_t fuel_percent,
                                    int16_t coolant_temp_c,
                                    int16_t outdoor_temp_c,
                                    uint16_t battery_mv,
                                    uint8_t validity_mask,
                                    uint32_t tick_ms);
/* 写入车身状态数据，同时记录最近一次 0x322 接收时间。 */
Std_ReturnType Rte_Write_BodyStatus(const Rte_BodyStatusType *body,
                                    uint32_t tick_ms);
/* 写入环境传感器快照。 */
Std_ReturnType Rte_Write_Environment(const Rte_EnvironmentDataType *environment);

/* 写入 TPMS 四轮胎压/胎温快照，同时记录最近一次 0x323 接收时间。 */
Std_ReturnType Rte_Write_TpmsStatus(const Rte_TpmsDataType *tpms, uint32_t tick_ms);

/* 写入 0x324 配置快照。 */
Std_ReturnType Rte_Write_ConfigData(const Rte_ConfigDataType *config, uint32_t tick_ms);

/* 写入 0x329 应用状态和 0x441 NM 状态。 */
Std_ReturnType Rte_Write_ControlDomainStatus(const Rte_ControlDomainStatusType *status, uint32_t tick_ms);
Std_ReturnType Rte_Write_ControlDomainNm(uint8_t node_address,
                                        uint8_t repeat_status,
                                        uint8_t power_on_request,
                                        uint8_t diag_request,
                                        uint32_t tick_ms);

/* 写入 RTC 时间和有效位，time 为空时会清除有效标志并返回 E_NOT_OK。 */
Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time, uint8_t valid);

/* 写入最近一次按键事件。 */
Std_ReturnType Rte_Write_KeyEvent(Rte_KeyEventType event);

/* 写入并挂起一条 0x327 用户输入事件，等待 Com 发送。 */
Std_ReturnType Rte_Write_UserInputEvent(const Rte_UserInputEventType *event);

/* 写入背光等级，超过 100 会被钳位。 */
Std_ReturnType Rte_Write_BacklightLevel(uint8_t level);

/* 写入蜂鸣器静音状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_BuzzerMuted(uint8_t muted);

/* 写入模拟模式状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_SimulationMode(uint8_t enabled);

/* 写入报警状态，非 0 统一归一化为 1。 */
Std_ReturnType Rte_Write_AlarmActive(uint8_t active);

/* 写入关机请求，非 0 归一化为 1。 */
Std_ReturnType Rte_Write_ShutdownRequest(uint8_t requested);

/* 清除车速/转速和动力域有效位，保留其它显示状态。 */
void Rte_Clear_DashboardValues(void);

/* 根据 CAN 超时阈值维护 can_ems_valid/can_body_valid。 */
void Rte_Update_CanValidity(uint32_t tick_ms);

/* 读取仪表数据快照。 */
Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data);

/* 读取环境数据快照。 */
Std_ReturnType Rte_Read_Environment(Rte_EnvironmentDataType *environment);

/* 读取 TPMS 数据快照。 */
Std_ReturnType Rte_Read_TpmsStatus(Rte_TpmsDataType *tpms);

/* 读取 0x324 配置快照。 */
Std_ReturnType Rte_Read_ConfigData(Rte_ConfigDataType *config);

/* 读取控制域聚合状态。 */
Std_ReturnType Rte_Read_ControlDomainStatus(Rte_ControlDomainStatusType *status);

/* 读取 RTC 时间和有效位。 */
Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time, uint8_t *valid);

/* 读取但不清除最近一次按键事件。 */
Std_ReturnType Rte_Read_KeyEvent(Rte_KeyEventType *event);

/* 读取并清除最近一次按键事件，适合只能消费一次的业务动作。 */
Std_ReturnType Rte_Take_KeyEvent(Rte_KeyEventType *event);

/* 读取并清除待发送的 0x327 用户输入事件。 */
Std_ReturnType Rte_Take_UserInputEvent(Rte_UserInputEventType *event);

/* 读取当前背光等级。 */
Std_ReturnType Rte_Read_BacklightLevel(uint8_t *level);

/* 返回最近一次动力域报文写入 RTE 的时间戳。 */
uint32_t Rte_GetLastPowertrainRxTick(void);

/* 快捷读取动力域 CAN 有效位。 */
uint8_t Rte_IsPowertrainValid(void);

/* 快捷读取 TPMS CAN 有效位。 */
uint8_t Rte_IsTpmsValid(void);

#endif /* RTE_SIGNAL_H */

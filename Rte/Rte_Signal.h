#ifndef RTE_SIGNAL_H
#define RTE_SIGNAL_H

#include "RtcIf.h"
#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    RTE_KEY_EVENT_NONE = 0u,
    RTE_KEY_EVENT_KEY1_SHORT,
    RTE_KEY_EVENT_KEY2_SHORT,
    RTE_KEY_EVENT_KEY3_SHORT,
    RTE_KEY_EVENT_POWER_LONG
} Rte_KeyEventType;

typedef struct
{
    uint16_t vehicle_speed_kph_x10;
    uint16_t engine_rpm;
    uint8_t fuel_percent;
    int16_t coolant_temp_c;
    int16_t outdoor_temp_c;
    uint16_t battery_mv;
    uint8_t ignition_status;
    uint8_t gear_position;
    uint8_t warning_flags;
    uint8_t can_ems_valid;
    uint8_t can_body_valid;
    uint8_t alarm_active;
    uint8_t buzzer_muted;
    uint8_t simulation_mode;
} Rte_DashboardDataType;

typedef struct
{
    int32_t temperature_c_x100;
    int32_t humidity_rh_x100;
    uint8_t valid;
} Rte_EnvironmentDataType;

void Rte_Signal_Init(void);

Std_ReturnType Rte_Write_Powertrain(uint16_t speed_kph_x10,
                                    uint16_t rpm,
                                    uint8_t fuel_percent,
                                    int16_t coolant_temp_c,
                                    int16_t outdoor_temp_c,
                                    uint16_t battery_mv,
                                    uint32_t tick_ms);
Std_ReturnType Rte_Write_BodyStatus(uint8_t ignition_status,
                                    uint8_t gear_position,
                                    uint8_t warning_flags,
                                    uint32_t tick_ms);
Std_ReturnType Rte_Write_Environment(const Rte_EnvironmentDataType *environment);
Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time, uint8_t valid);
Std_ReturnType Rte_Write_KeyEvent(Rte_KeyEventType event);
Std_ReturnType Rte_Write_BacklightLevel(uint8_t level);
Std_ReturnType Rte_Write_BuzzerMuted(uint8_t muted);
Std_ReturnType Rte_Write_SimulationMode(uint8_t enabled);
Std_ReturnType Rte_Write_AlarmActive(uint8_t active);
void Rte_Clear_DashboardValues(void);
void Rte_Update_CanValidity(uint32_t tick_ms);

Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data);
Std_ReturnType Rte_Read_Environment(Rte_EnvironmentDataType *environment);
Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time, uint8_t *valid);
Std_ReturnType Rte_Read_KeyEvent(Rte_KeyEventType *event);
Std_ReturnType Rte_Take_KeyEvent(Rte_KeyEventType *event);
Std_ReturnType Rte_Read_BacklightLevel(uint8_t *level);

uint32_t Rte_GetLastPowertrainRxTick(void);
uint8_t Rte_IsPowertrainValid(void);

#endif /* RTE_SIGNAL_H */
#ifndef RTE_SIGNAL_H
#define RTE_SIGNAL_H

#include "RtcIf.h"
#include "Std_Types.h"

#include <stdint.h>

typedef struct
{
    uint16_t speed_kph_x10;
    uint16_t engine_rpm;
    uint8_t fuel_percent;
    int16_t coolant_temp_c;
    int16_t outdoor_temp_c;
    uint16_t battery_mv;
    uint8_t ignition_status;
    uint8_t gear_position;
    uint8_t warning_flags;
    uint8_t door_flags;
    uint8_t config_theme;
    uint8_t backlight_level;
    int16_t sht30_temp_c_x10;
    uint16_t sht30_humidity_x10;
    RtcIf_TimeType rtc_time;
    uint8_t key_event;
    uint8_t buzzer_enable;
    uint8_t buzzer_alarm;
    uint8_t can_ems_valid;
    uint8_t sensor_valid;
    uint8_t rtc_valid;
} Rte_DashboardDataType;

void Rte_SignalInit(void);
void Rte_SignalMainFunction(uint16_t elapsed_ms);

Std_ReturnType Rte_Write_VehicleSpeed(uint16_t speed_kph_x10);
Std_ReturnType Rte_Write_EngineRpm(uint16_t rpm);
Std_ReturnType Rte_Write_FuelPercent(uint8_t fuel_percent);
Std_ReturnType Rte_Write_CoolantTemp(int16_t temp_c);
Std_ReturnType Rte_Write_OutdoorTemp(int16_t temp_c);
Std_ReturnType Rte_Write_BatteryVoltage(uint16_t battery_mv);
Std_ReturnType Rte_Write_IgnitionStatus(uint8_t ignition_status);
Std_ReturnType Rte_Write_GearPosition(uint8_t gear_position);
Std_ReturnType Rte_Write_WarningFlags(uint8_t warning_flags);
Std_ReturnType Rte_Write_DoorFlags(uint8_t door_flags);
Std_ReturnType Rte_Write_ConfigTheme(uint8_t theme);
Std_ReturnType Rte_Write_BacklightLevel(uint8_t level);
Std_ReturnType Rte_Write_Sht30(int16_t temp_c_x10, uint16_t humidity_x10);
Std_ReturnType Rte_Write_RtcTime(const RtcIf_TimeType *time);
Std_ReturnType Rte_Write_KeyEvent(uint8_t key_event);
Std_ReturnType Rte_Write_BuzzerEnable(uint8_t enable);
Std_ReturnType Rte_Write_BuzzerAlarm(uint8_t alarm);
void Rte_MarkCanEmsReceived(void);

Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data);
Std_ReturnType Rte_Read_VehicleSpeed(uint16_t *speed_kph_x10);
Std_ReturnType Rte_Read_EngineRpm(uint16_t *rpm);
Std_ReturnType Rte_Read_BatteryVoltage(uint16_t *battery_mv);
Std_ReturnType Rte_Read_RtcTime(RtcIf_TimeType *time);

uint16_t Rte_GetCanEmsAgeMs(void);
uint8_t Rte_IsCanEmsValid(void);

#endif /* RTE_SIGNAL_H */
